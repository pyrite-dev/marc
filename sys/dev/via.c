#include <sys/device.h>
#include <sys/interrupt.h>
#include <sys/bus.h>
#include <sys/sched.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "via.h"

struct via_softc {
	bus_space_t space;

	int vector;

	unsigned int timer_ticks;
	unsigned int vbl_ticks;
};

static void via_interrupt(unsigned int vector, struct iregs* iregs, void* arg) {
	struct via_softc* softc = ((struct device*)arg)->softc;
	uint8_t		  ifr	= bus_space_read8(&softc->space, VIA_IFR);

	/* writing a one to a flag bit clears it; bit 7 is read-only */
	bus_space_write8(&softc->space, VIA_IFR, 0xff);

	if(ifr & VIA_INT_TIMER1) {
		softc->timer_ticks++;
		sched_tick();
	}

	if(ifr & VIA_INT_VBLANK) {
		softc->vbl_ticks++;
	}
}

/* Put timer 1 into free-run mode. */
static void via_timer_init(struct via_softc* softc, unsigned int count) {
	/* stop all VIA interrupts while the timer is reprogrammed */
	bus_space_write8(&softc->space, VIA_IER, 0x7f);

	/* timer 1: free-run, with the PB7 output disabled */
	bus_space_write8(&softc->space, VIA_ACR,
			 (bus_space_read8(&softc->space, VIA_ACR) & ~0x80) | VIA_ACR_TIMER1_FREERUN);

	bus_space_write8(&softc->space, VIA_T1LL, count & 0xff);
	bus_space_write8(&softc->space, VIA_T1LH, (count >> 8) & 0xff);
	bus_space_write8(&softc->space, VIA_T1CL, count & 0xff);
	bus_space_write8(&softc->space, VIA_T1CH, (count >> 8) & 0xff);

	/* discard anything that became pending while the timer was programmed */
	bus_space_write8(&softc->space, VIA_IFR, 0xff);
}

static int via_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int via_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct via_softc* softc;
	unsigned int	  clock	 = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_CLOCK], 392000);
	unsigned int	  speed	 = DEVICE_LOCATOR_DEFAULT(args->locators[DEVICE_LOCATOR_SPEED], 200);
	unsigned int	  count;
	int		  vector = _intr_vector(args->locators[DEVICE_LOCATOR_IRQ]);

	if(speed == 0) return -1;

	count = clock / speed;

	if(count == 0 || count > 65535) return -1;
	if(vector < 0) return -1;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));
	softc->vector = vector;

	bus_space_init(&softc->space, args->locators);

	via_timer_init(softc, count);

	hz = clock / count;

	return 0;
}

static void via_enable_ints(struct device* self) {
	struct via_softc* softc = self->softc;

	intr_register(softc->vector, via_interrupt, self);
	intr_enable(softc->vector);

	bus_space_write8(&softc->space, VIA_IER, VIA_IER_SET | VIA_INT_TIMER1 | VIA_INT_VBLANK);
}

static void via_detach(struct device* self) {
	struct via_softc* softc = self->softc;

	if(softc == NULL) return;

	bus_space_write8(&softc->space, VIA_IER, 0x7f); /* writing without bit 7 clears enables */
	intr_disable(softc->vector);
	intr_unregister(softc->vector);

	free(softc);
	self->softc = NULL;
}

struct device_driver via_driver = {
    "via",
    via_match,
    via_attach,
    via_enable_ints,
    via_detach};
