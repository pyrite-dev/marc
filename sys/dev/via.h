#ifndef __SYS_DEV_VIA_H__
#define __SYS_DEV_VIA_H__

/* 6522 register numbers */
#define VIA_ORB 0
#define VIA_ORA 1
#define VIA_DDRB 2
#define VIA_DDRA 3
#define VIA_T1CL 4
#define VIA_T1CH 5
#define VIA_T1LL 6
#define VIA_T1LH 7
#define VIA_T2CL 8
#define VIA_T2CH 9
#define VIA_SR 10
#define VIA_ACR 11
#define VIA_PCR 12
#define VIA_IFR 13
#define VIA_IER 14

/* ACR bit 6 selects timer 1 free-run mode (bit 7 is the PB7 output enable) */
#define VIA_ACR_TIMER1_FREERUN 0x40

/* bit 7 set when writing IER means "enable these interrupts" */
#define VIA_IER_SET 0x80

#define VIA_INT_IRQ 0x80 /* any enabled VIA interrupt is pending */
#define VIA_INT_TIMER1 0x40
#define VIA_INT_TIMER2 0x20
#define VIA_INT_KBCLK 0x10  /* keyboard clock (CB1) */
#define VIA_INT_KBDAT 0x08  /* keyboard data (CB2) */
#define VIA_INT_KBRDY 0x04  /* keyboard shift register full */
#define VIA_INT_VBLANK 0x02 /* vertical retrace (CA1), about 60.15 Hz */
#define VIA_INT_ONESEC 0x01

#endif
