#include <sys/cons.h>

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <machine/bootinfo.h>
#include <machine/mac_hw.h>

#include "font.h"

/* pixels are set for black and clear for white, as on the Mac's 1-bit screen */
static volatile uint8_t* screen_base;
static uint32_t		 screen_row_bytes;
static uint32_t		 screen_width;
static uint32_t		 screen_height;
static uint32_t		 console_cols;
static uint32_t		 console_rows;

static int console_x;
static int console_y;

static void console_putc(int c);
static int  console_getc(void);

static void glyph_row(int cx, int cy, const uint8_t* glyph, int row) {
	volatile uint8_t* dest_row = screen_base + (cy * FONT_HEIGHT + row) * screen_row_bytes;
	uint8_t		  ink	   = glyph[row]; /* 6 significant bits, leftmost in bit 7 */
	int		  i;

	for(i = 0; i < FONT_WIDTH; i++) {
		int	px   = cx * FONT_WIDTH + i;
		int	bit  = px % 8;
		uint8_t mask = 0x80 >> bit;

		if(ink & (0x80 >> i)) {
			dest_row[px / 8] |= mask;
		} else {
			dest_row[px / 8] &= ~mask;
		}
	}
}

static void draw_glyph(int cx, int cy, int c) {
	const uint8_t* glyph;
	int	       row;

	glyph = &font[(c - 32) * FONT_HEIGHT];

	for(row = 0; row < FONT_HEIGHT; row++) {
		glyph_row(cx, cy, glyph, row);
	}
}

static void clear_glyph(int cx, int cy) {
	int row;

	for(row = 0; row < FONT_HEIGHT; row++) {
		volatile uint8_t* dest_row = screen_base + (cy * FONT_HEIGHT + row) * screen_row_bytes;
		int		  i;

		for(i = 0; i < FONT_WIDTH; i++) {
			int	px   = cx * FONT_WIDTH + i;
			uint8_t mask = 0x80 >> (px % 8);

			dest_row[px / 8] &= ~mask;
		}
	}
}

static void newline(void) {
	console_x = 0;
	console_y++;

	if(console_y >= (int)console_rows) {
		memmove((void*)screen_base, (void*)(screen_base + FONT_HEIGHT * screen_row_bytes), (screen_height - FONT_HEIGHT) * screen_row_bytes);
		memset((void*)(screen_base + (screen_height - FONT_HEIGHT) * screen_row_bytes), 0x00, FONT_HEIGHT * screen_row_bytes);
		console_y = (int)console_rows - 1;
	}
}

void cons_init(void) {
	screen_width  = boot_screen_width;
	screen_height = boot_screen_height;

	if(screen_width == 0 || screen_height == 0) {
		/* the boot block couldn't tell us, so fall back to the numbers a Plus expects */
		screen_width  = 512;
		screen_height = 342;
	}

	screen_row_bytes = screen_width / 8;

	console_cols = screen_width / FONT_WIDTH;
	console_rows = screen_height / FONT_HEIGHT;

	screen_base = (volatile uint8_t*)(uintptr_t)MAC_SCRNBASE;

	memset((void*)screen_base, 0x00, screen_height * screen_row_bytes);

	console_x = 0;
	console_y = 0;

	cons_output_register(console_putc);
	cons_input_register(console_getc);
}

static void console_putc(int c) {
	if(c == '\r') {
		console_x = 0;
		return;
	}

	if(c == '\n') {
		newline();
		return;
	}

	if(c == '\b') {
		if(console_x > 0) {
			console_x--;
			clear_glyph(console_x, console_y);
		}
		return;
	}

	if(c < ' ' || c > 126) return; /* the font only covers printable ASCII */

	draw_glyph(console_x, console_y, c);

	console_x++;
	if(console_x >= (int)console_cols) newline();
}

static int console_getc(void) {
	return -1; /* keyboard input is not wired up yet */
}
