#ifndef __SCC_H__
#define __SCC_H__

/* Register numbers */
#define SCC_RR0 0
#define SCC_RR1 1
#define SCC_RR2 2
#define SCC_RR3 3

#define SCC_WR0 0
#define SCC_WR1 1
#define SCC_WR2 2
#define SCC_WR3 3
#define SCC_WR4 4
#define SCC_WR5 5
#define SCC_WR6 6
#define SCC_WR7 7
#define SCC_WR8 8
#define SCC_WR9 9
#define SCC_WR10 10
#define SCC_WR11 11
#define SCC_WR12 12
#define SCC_WR13 13
#define SCC_WR14 14
#define SCC_WR15 15

/* WR0 commands */
#define SCC_CMD(cmd) ((cmd) << 3)
#define SCC_CMD_NULL 0
#define SCC_CMD_POINT_HIGH 1
#define SCC_CMD_RESET_EXT 2
#define SCC_CMD_RESET_TX 5
#define SCC_CMD_RESET_ERROR 6
#define SCC_CMD_RESET_IUS 7

/* RR0 */
#define SCC_RR0_RX_AVAIL 0x01
#define SCC_RR0_TX_EMPTY 0x04

/* WR1 */
#define SCC_WR1_EXT_IE 0x01
#define SCC_WR1_TX_IE 0x02
#define SCC_WR1_RX_IE(mode) ((mode) << 3)

/* WR3 */
#define SCC_WR3_RX_ENABLE 0x01
#define SCC_WR3_8BIT 0xc0

/* WR4 */
#define SCC_WR4_1_STOP 0x04
#define SCC_WR4_2_STOP 0x0c
#define SCC_WR4_X16 0x40
#define SCC_WR4_ASYNC_8N1 (SCC_WR4_X16 | SCC_WR4_1_STOP)

/* WR5 */
#define SCC_WR5_TX_ENABLE 0x08
#define SCC_WR5_RTS 0x02
#define SCC_WR5_8BIT 0x60
#define SCC_WR5_ASYNC_8N1 (SCC_WR5_8BIT | SCC_WR5_TX_ENABLE | SCC_WR5_RTS)

/* WR9 */
#define SCC_WR9_RESET_B 0x40
#define SCC_WR9_RESET_A 0x80
#define SCC_WR9_RESET 0xc0
#define SCC_WR9_MIE 0x08

/* WR11 */
#define SCC_WR11_RX_BRG 0x40
#define SCC_WR11_TX_BRG 0x10

/* WR14 */
#define SCC_WR14_BRG_ENABLE 0x01

/* Channels */
#define SCC_CHANNEL_B 0
#define SCC_CHANNEL_A 1

/* Byte offsets from the read/write base */
#define SCC_CTL(ch) ((ch) == SCC_CHANNEL_A ? 2 : 0)
#define SCC_DATA(ch) ((ch) == SCC_CHANNEL_A ? 6 : 4)

#endif
