#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/cons.h>
#include <sys/sched.h>

#include <stddef.h>
#include <stdint.h>

#include <machine/mac_hw.h>

#define VIA(reg) (*(volatile uint8_t*)(via_base + 512 * (reg)))
#define IER_SET 0x80 /* bit 7 set when writing IER means "enable these interrupts" */

#define VIA_TICK_MS (1000 / 200)
#define VIA_TIMER_COUNT (MAC_VIA_TICKS_PER_MS * VIA_TICK_MS)

static uint32_t		 via_base;
static int			 via_vector = -1;
static volatile uint32_t timer_ticks;
static volatile uint32_t vbl_ticks;

static void via_interrupt(unsigned int vector, struct iregs* iregs, void* arg) {
	uint8_t ifr = VIA(MAC_VIA_REG_IFR);

	/* writing a one to a flag bit clears it; bit 7 is read-only */
	VIA(MAC_VIA_REG_IFR) = 0xff;

	if(ifr & MAC_VIA_INT_TIMER1) {
		timer_ticks++;
		sched_tick();
	}

	if(ifr & MAC_VIA_INT_VBLANK) {
		vbl_ticks++;
	}
}

/* Put timer 1 into free-run mode. */
static void via_timer_init(void) {
	uint16_t count = VIA_TIMER_COUNT;

	/* stop all VIA interrupts while the timer is reprogrammed */
	VIA(MAC_VIA_REG_IER) = 0x7f;

	/* timer 1: free-run, with the PB7 output disabled */
	VIA(MAC_VIA_REG_ACR) = (VIA(MAC_VIA_REG_ACR) & ~0x80) | MAC_VIA_ACR_TIMER1_FREERUN;

	VIA(MAC_VIA_REG_T1LL) = count & 0xff;
	VIA(MAC_VIA_REG_T1LH) = (count >> 8) & 0xff;
	VIA(MAC_VIA_REG_T1CL) = count & 0xff;
	VIA(MAC_VIA_REG_T1CH) = (count >> 8) & 0xff;

	/* discard anything that became pending while the timer was programmed */
	VIA(MAC_VIA_REG_IFR) = 0xff;
}

static int via_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int via_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	via_base   = (uint32_t)args->locators[DEVICE_LOCATOR_ADDR];
	via_vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);

	timer_ticks = 0;
	vbl_ticks   = 0;

	if(via_vector < 0) return -1;

	via_timer_init();

	intr_register(via_vector, via_interrupt, NULL);
	intr_enable(via_vector);

	VIA(MAC_VIA_REG_IER) = IER_SET | MAC_VIA_INT_TIMER1 | MAC_VIA_INT_VBLANK;

	return 0;
}

static void via_detach(struct device* self) {
	VIA(MAC_VIA_REG_IER) = 0x7f; /* writing without bit 7 clears enables */
	if(via_vector >= 0) {
		intr_disable(via_vector);
		intr_unregister(via_vector);
	}
}

struct device_driver via_driver = {
    "via",
    via_match,
    via_attach,
    via_detach};
