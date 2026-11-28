#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/spinlock.h>
#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/errno.h>

#include "scc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct scc_softc {
	struct tty tty[2]; /* A, B: tty1 and tty2 */

	bus_space_t rspace;
	bus_space_t wspace;

	spinlock_t lock;

	unsigned int clock;
	unsigned int vector;
};

static struct tty* scc_tty(struct scc_softc* softc, int channel) {
	return &softc->tty[channel == SCC_CHANNEL_A ? 0 : 1];
}

static int scc_channel(struct scc_softc* softc, struct tty* tty) {
	return tty == &softc->tty[0] ? SCC_CHANNEL_A : SCC_CHANNEL_B;
}

static void scc_delay(void) {
	__asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop");
}

static uint8_t scc_read_ctrl(struct scc_softc* softc, int channel) {
	return bus_space_read8(&softc->rspace, SCC_CTL(channel));
}

static void scc_write_ctrl(struct scc_softc* softc, int channel, uint8_t value) {
	bus_space_write8(&softc->wspace, SCC_CTL(channel), value);
}

static uint8_t scc_read_data(struct scc_softc* softc, int channel) {
	return bus_space_read8(&softc->rspace, SCC_DATA(channel));
}

static void scc_write_data(struct scc_softc* softc, int channel, uint8_t value) {
	bus_space_write8(&softc->wspace, SCC_DATA(channel), value);
}

static uint8_t scc_read_reg_locked(struct scc_softc* softc, int channel, int reg) {
	scc_write_ctrl(softc, channel, reg);
	scc_delay();

	return scc_read_ctrl(softc, channel);
}

static void scc_write_reg_locked(struct scc_softc* softc, int channel, int reg, uint8_t value) {
	scc_write_ctrl(softc, channel, reg);
	scc_delay();
	scc_write_ctrl(softc, channel, value);
	scc_delay();
}

static unsigned int scc_brg_constant(unsigned int clock, unsigned int baud) {
	unsigned int div;

	if(baud == 0) return 0;

	div = (clock + 16 * baud) / (32 * baud);
	if(div < 2) div = 2;

	return div - 2;
}

static void scc_set_baud_locked(struct scc_softc* softc, int channel, unsigned int baud) {
	unsigned int constant = scc_brg_constant(softc->clock, baud);

	scc_write_reg_locked(softc, channel, SCC_WR12, constant & 0xff);
	scc_write_reg_locked(softc, channel, SCC_WR13, (constant >> 8) & 0xff);
	scc_write_reg_locked(softc, channel, SCC_WR14, SCC_WR14_BRG_ENABLE);
	scc_write_reg_locked(softc, channel, SCC_WR11, SCC_WR11_RX_BRG | SCC_WR11_TX_BRG);
}

static int scc_tx_empty(struct scc_softc* softc, int channel) {
	intr_flags_t flags;
	uint8_t	     rr0;

	flags = spinlock_lock_irqsave(&softc->lock);
	rr0   = scc_read_reg_locked(softc, channel, SCC_RR0);
	spinlock_unlock_irqrestore(&softc->lock, flags);

	return (rr0 & SCC_RR0_TX_EMPTY) != 0;
}

static int scc_rx_char(struct scc_softc* softc, int channel, uint8_t* data) {
	intr_flags_t flags;
	uint8_t	     rr0;

	flags = spinlock_lock_irqsave(&softc->lock);
	rr0   = scc_read_reg_locked(softc, channel, SCC_RR0);

	if(rr0 & SCC_RR0_RX_AVAIL) {
		*data = scc_read_data(softc, channel);
		spinlock_unlock_irqrestore(&softc->lock, flags);
		return 1;
	}

	spinlock_unlock_irqrestore(&softc->lock, flags);

	return 0;
}

static void scc_write_byte(struct scc_softc* softc, int channel, uint8_t data) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);

	scc_write_data(softc, channel, data);

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void scc_reset_tx_int(struct scc_softc* softc, int channel) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);

	scc_write_reg_locked(softc, channel, SCC_WR0, SCC_CMD(SCC_CMD_RESET_TX));

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void scc_reset_ext_int(struct scc_softc* softc, int channel) {
	intr_flags_t flags = spinlock_lock_irqsave(&softc->lock);

	scc_write_reg_locked(softc, channel, SCC_WR0, SCC_CMD(SCC_CMD_RESET_EXT));

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void scc_start(struct tty* tty) {
	struct scc_softc* softc = tty->device->softc;
	int		  channel = scc_channel(softc, tty);
	unsigned char	  data;

	if(!scc_tx_empty(softc, channel)) return;
	if(tty_output_getc(tty, &data)) scc_write_byte(softc, channel, data);
}

static int scc_param(struct tty* tty, const struct termios* termios) {
	struct scc_softc* softc = tty->device->softc;
	int		  channel = scc_channel(softc, tty);
	unsigned int	  baud = termios->c_ospeed;
	intr_flags_t	  flags;

	if(baud == B0) return 0;
	if((termios->c_cflag & CSIZE) != CS8) return EINVAL;
	if(termios->c_cflag & (CSTOPB | PARENB)) return EINVAL;
	if(termios->c_ispeed != 0 && termios->c_ispeed != termios->c_ospeed) return EINVAL;

	flags = spinlock_lock_irqsave(&softc->lock);

	scc_set_baud_locked(softc, channel, baud);

	spinlock_unlock_irqrestore(&softc->lock, flags);

	return 0;
}

static void scc_putc(struct tty* tty, int c) {
	struct scc_softc* softc = tty->device->softc;
	int		  channel = scc_channel(softc, tty);

	while(!scc_tx_empty(softc, channel));

	scc_write_byte(softc, channel, (uint8_t)c);
}

static const struct tty_ops scc_tty_ops = {
    scc_start,
    scc_param,
    NULL,
    scc_putc};

static void scc_intr(unsigned int vector, struct iregs* iregs, void* arg) {
	struct scc_softc* softc = ((struct device*)arg)->softc;
	int		  i;

	for(i = 0; i < 64; i++) {
		int channel;
		int did = 0;

		for(channel = SCC_CHANNEL_B; channel <= SCC_CHANNEL_A; channel++) {
			uint8_t data;
			struct tty* tty;

			tty = scc_tty(softc, channel);

			if(scc_rx_char(softc, channel, &data)) {
				tty_input(tty, data, 0);
				did = 1;
			}

			if(scc_tx_empty(softc, channel)) {
				unsigned char out;

				if(tty_output_getc(tty, &out)) {
					scc_write_byte(softc, channel, out);
					did = 1;
				} else {
					scc_reset_tx_int(softc, channel);
				}
			}
		}

		if(!did) break;
	}

	scc_reset_ext_int(softc, SCC_CHANNEL_B);
	scc_reset_ext_int(softc, SCC_CHANNEL_A);
}

static int scc_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static void scc_init_channel(struct scc_softc* softc, int channel, unsigned int baud) {
	scc_write_reg_locked(softc, channel, SCC_WR9, channel == SCC_CHANNEL_A ? SCC_WR9_RESET_A : SCC_WR9_RESET_B);
	scc_write_reg_locked(softc, channel, SCC_WR4, SCC_WR4_ASYNC_8N1);
	scc_write_reg_locked(softc, channel, SCC_WR3, SCC_WR3_8BIT | SCC_WR3_RX_ENABLE);
	scc_write_reg_locked(softc, channel, SCC_WR5, SCC_WR5_ASYNC_8N1);
	scc_write_reg_locked(softc, channel, SCC_WR1, SCC_WR1_TX_IE | SCC_WR1_RX_IE(2));
	scc_write_reg_locked(softc, channel, SCC_WR2, 0);
	scc_set_baud_locked(softc, channel, baud);
	scc_write_reg_locked(softc, channel, SCC_WR9, 0);
}

static int scc_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct scc_softc* softc;
	unsigned int	  clock = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_CLOCK], 3672000);
	unsigned int	  baud  = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_BAUD], 9600);
	intr_flags_t	  flags;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	spinlock_init(&softc->lock);

	bus_space_init(&softc->rspace, args->locators);
	bus_space_init(&softc->wspace, args->locators);

	softc->wspace.addr = args->locators[DEVICE_LOCATOR_PORT] == DEVICE_LOCATOR_UNSET
				 ? 0xbffff9
				 : args->locators[DEVICE_LOCATOR_PORT];
	softc->clock  = clock;
	softc->vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);

	if(softc->vector < 0) {
		free(softc);
		self->softc = NULL;
		return -1;
	}

	flags = spinlock_lock_irqsave(&softc->lock);

	/* hardware reset of both channels */
	scc_write_reg_locked(softc, SCC_CHANNEL_B, SCC_WR9, SCC_WR9_RESET);
	scc_delay();

	scc_init_channel(softc, SCC_CHANNEL_A, baud);
	scc_init_channel(softc, SCC_CHANNEL_B, baud);

	spinlock_unlock_irqrestore(&softc->lock, flags);

	if(ttydev_init(self, &softc->tty[0], NULL, &scc_tty_ops, baud) == DEVICE_ERROR) {
		free(softc);
		self->softc = NULL;
		return -1;
	}

	if(ttydev_init(self, &softc->tty[1], NULL, &scc_tty_ops, baud) == DEVICE_ERROR) {
		ttydev_detach(self);
		free(softc);
		self->softc = NULL;
		return -1;
	}

	return 0;
}

static void scc_enable_ints(struct device* self) {
	struct scc_softc* softc = self->softc;
	intr_flags_t	  flags;

	intr_register(softc->vector, scc_intr, self);
	intr_enable(softc->vector);

	flags = spinlock_lock_irqsave(&softc->lock);

	scc_write_reg_locked(softc, SCC_CHANNEL_A, SCC_WR9, SCC_WR9_MIE);
	scc_write_reg_locked(softc, SCC_CHANNEL_B, SCC_WR9, SCC_WR9_MIE);

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void scc_detach(struct device* self) {
	struct scc_softc* softc = self->softc;
	intr_flags_t	  flags;

	if(softc == NULL) return;

	intr_disable(softc->vector);

	flags = spinlock_lock_irqsave(&softc->lock);

	scc_write_reg_locked(softc, SCC_CHANNEL_A, SCC_WR9, 0);
	scc_write_reg_locked(softc, SCC_CHANNEL_B, SCC_WR9, 0);

	spinlock_unlock_irqrestore(&softc->lock, flags);

	intr_unregister(softc->vector);
	ttydev_detach(self);

	free(softc);
	self->softc = NULL;
}

struct device_driver scc_driver = {
    "scc",
    scc_match,
    scc_attach,
    scc_enable_ints,
    scc_detach};
