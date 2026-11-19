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

/* SCC (Z8530) */
#define MAC_SCC_RD_BASE 0x9ffff8
#define MAC_SCC_WR_BASE 0xbffff9
#define MAC_SCC_B_CTL_RD (*(volatile uint8_t*)(MAC_SCC_RD_BASE + 0x0))
#define MAC_SCC_B_CTL_WR (*(volatile uint8_t*)(MAC_SCC_WR_BASE + 0x0))

#endif
