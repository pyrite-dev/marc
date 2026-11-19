#ifndef __MACHINE_MAC_HW_H__
#define __MACHINE_MAC_HW_H__

#include <stdint.h>

/* low memory globals maintained by the Mac ROM */
#define MAC_MEMTOP (*(volatile uint32_t*)0x0108)   /* end of RAM */
#define MAC_SCRNBASE (*(volatile uint32_t*)0x0824) /* main screen buffer */

#define MAC_VIDEO_RESERVE 0xd900

/* VIA 6522 (VIA1) */
#define MAC_VIA_BASE 0xefe1fe
#define MAC_VIA(n) (*(volatile uint8_t*)(MAC_VIA_BASE + 512 * (n)))

/* 6522 register offsets */
#define MAC_VIA_REG_ORB  0
#define MAC_VIA_REG_ORA  1
#define MAC_VIA_REG_DDRB 2
#define MAC_VIA_REG_DDRA 3
#define MAC_VIA_REG_T1CL 4
#define MAC_VIA_REG_T1CH 5
#define MAC_VIA_REG_T1LL 6
#define MAC_VIA_REG_T1LH 7
#define MAC_VIA_REG_T2CL 8
#define MAC_VIA_REG_T2CH 9
#define MAC_VIA_REG_SR	  10
#define MAC_VIA_REG_ACR	  11
#define MAC_VIA_REG_PCR	  12
#define MAC_VIA_REG_IFR	  13
#define MAC_VIA_REG_IER	  14

/* ACR bit 6 selects timer 1 free-run mode (bit 7 is the PB7 output enable) */
#define MAC_VIA_ACR_TIMER1_FREERUN 0x40

/* uClinux's Mac Plus VIA_TIME() constant: 392 timer ticks per millisecond */
#define MAC_VIA_TICKS_PER_MS 392

#define MAC_VIA_INT_IRQ 0x80 /* any enabled VIA interrupt is pending */
#define MAC_VIA_INT_TIMER1 0x40
#define MAC_VIA_INT_TIMER2 0x20
#define MAC_VIA_INT_KBCLK 0x10	/* keyboard clock (CB1) */
#define MAC_VIA_INT_KBDAT 0x08	/* keyboard data (CB2) */
#define MAC_VIA_INT_KBRDY 0x04	/* keyboard shift register full */
#define MAC_VIA_INT_VBLANK 0x02 /* vertical retrace (CA1), about 60.15 Hz */
#define MAC_VIA_INT_ONESEC 0x01

/* SCC (Z8530) */
#define MAC_SCC_RD_BASE 0x9ffff8
#define MAC_SCC_WR_BASE 0xbffff9
#define MAC_SCC_B_CTL_RD (*(volatile uint8_t*)(MAC_SCC_RD_BASE + 0x0))
#define MAC_SCC_B_CTL_WR (*(volatile uint8_t*)(MAC_SCC_WR_BASE + 0x0))

#endif
