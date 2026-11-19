#include <sys/machdep.h>
#include <sys/kmem.h>

#include <stddef.h>
#include <stdint.h>

#include <machine/bootinfo.h>
#include <machine/mac_hw.h>

extern char _end;

/* set by _start from the arguments that the boot block passes in */
uint32_t    boot_screen_width;
uint32_t    boot_screen_height;
const char* boot_cmdline;
char	    boot_args[BOOT_ARGS_MAX];

static void scc_delay(void) {
	__asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop");
}

/* the ROM may have left the SCC's interrupts enabled, which would keep interrupt level 2 asserted */
static void scc_reset(void) {
	(void)MAC_SCC_B_CTL_RD; /* reading resets the register pointer to register 0 */
	scc_delay();
	MAC_SCC_B_CTL_WR = 9; /* select WR9 */
	scc_delay();
	MAC_SCC_B_CTL_WR = 0xc0; /* hardware reset of both channels */
	scc_delay();
}

void machdep_init(void) {
	int i;

	for(i = 0; boot_cmdline != NULL && i < BOOT_ARGS_MAX - 1 && boot_cmdline[i] != 0; i++) {
		boot_args[i] = boot_cmdline[i];
	}
	boot_args[i] = 0;

	MAC_VIA(MAC_VIA_REG_IER) = 0x7f; /* disable all VIA interrupts until a driver asks for them */
	MAC_VIA(MAC_VIA_REG_IFR) = 0x7f; /* and clear anything that is already pending */

	scc_reset();

	kmem_init(&_end, (void*)(uintptr_t)(MAC_MEMTOP - MAC_VIDEO_RESERVE));
}
