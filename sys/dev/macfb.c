#include <sys/device.h>
#include <sys/spinlock.h>
#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/font.h>

#include <machine/mac_hw.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MACFB_COLS(width) ((width) / 8)
#define MACFB_ROWS(height) ((height) / 8)

extern uint32_t boot_screen_width;
extern uint32_t boot_screen_height;

struct macfb_softc {
	struct tty tty;

	spinlock_t lock;

	volatile uint8_t* screen;

	unsigned int width;
	unsigned int height;
	unsigned int row_bytes;
	unsigned int cols;
	unsigned int rows;

	int x;
	int y;
};

static void macfb_glyph_row(struct macfb_softc* softc, int cx, int cy, const uint8_t* glyph, int row) {
	volatile uint8_t* dest = softc->screen + (cy * 8 + row) * softc->row_bytes;
	uint8_t		  ink  = glyph[row];
	int		  i;

	for(i = 0; i < 8; i++) {
		int	px   = cx * 8 + i;
		uint8_t mask = 0x80 >> (px % 8);

		if(ink & (1 << i)) {
			dest[px / 8] &= ~mask;
		} else {
			dest[px / 8] |= mask;
		}
	}
}

static void macfb_draw_glyph(struct macfb_softc* softc, int c) {
	const uint8_t* glyph = font[c];
	int	       row;

	for(row = 0; row < 8; row++) {
		macfb_glyph_row(softc, softc->x, softc->y, glyph, row);
	}
}

static void macfb_clear_glyph(struct macfb_softc* softc, int cx, int cy) {
	int row;

	for(row = 0; row < 8; row++) {
		volatile uint8_t* dest = softc->screen + (cy * 8 + row) * softc->row_bytes;
		int		  i;

		for(i = 0; i < 8; i++) {
			int	px   = cx * 8 + i;
			uint8_t mask = 0x80 >> (px % 8);

			dest[px / 8] |= mask;
		}
	}
}

static void macfb_newline(struct macfb_softc* softc) {
	softc->x = 0;
	softc->y++;

	if(softc->y >= (int)softc->rows) {
		memmove((void*)softc->screen,
			(void*)(softc->screen + 8 * softc->row_bytes),
			(softc->height - 8) * softc->row_bytes);
		memset((void*)(softc->screen + (softc->height - 8) * softc->row_bytes), 0xff, 8 * softc->row_bytes);
		softc->y = (int)softc->rows - 1;
	}
}

static void macfb_putc(struct tty* tty, int c) {
	struct macfb_softc* softc = tty->device->softc;
	intr_flags_t	    flags = spinlock_lock_irqsave(&softc->lock);

	if(c == '\r') {
		softc->x = 0;
		spinlock_unlock_irqrestore(&softc->lock, flags);
		return;
	}

	if(c == '\n') {
		macfb_newline(softc);
		spinlock_unlock_irqrestore(&softc->lock, flags);
		return;
	}

	if(c == '\b') {
		if(softc->x > 0) {
			softc->x--;
			macfb_clear_glyph(softc, softc->x, softc->y);
		}
		spinlock_unlock_irqrestore(&softc->lock, flags);
		return;
	}

	if(c < ' ' || c > 126) {
		spinlock_unlock_irqrestore(&softc->lock, flags);
		return;
	}

	macfb_draw_glyph(softc, c);

	softc->x++;
	if(softc->x >= (int)softc->cols) macfb_newline(softc);

	spinlock_unlock_irqrestore(&softc->lock, flags);
}

static void macfb_start(struct tty* tty) {
	unsigned char c;

	while(tty_output_getc(tty, &c)) macfb_putc(tty, c);
}

static const struct tty_ops macfb_tty_ops = {
    macfb_start,
    NULL,
    NULL,
    macfb_putc};

static int macfb_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int macfb_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	struct macfb_softc* softc;

	self->softc = softc = malloc(sizeof(*softc));
	if(softc == NULL) return -1;

	memset(softc, 0, sizeof(*softc));

	spinlock_init(&softc->lock);

	softc->width  = boot_screen_width;
	softc->height = boot_screen_height;

	if(softc->width == 0 || softc->height == 0) {
		softc->width  = 512;
		softc->height = 342;
	}

	softc->row_bytes = softc->width / 8;
	softc->cols	 = MACFB_COLS(softc->width);
	softc->rows	 = MACFB_ROWS(softc->height);
	softc->screen	 = (volatile uint8_t*)(uintptr_t)MAC_SCRNBASE;

	memset((void*)softc->screen, 0xff, softc->height * softc->row_bytes);

	if(ttydev_init(self, &softc->tty, NULL, &macfb_tty_ops, 9600) == DEVICE_ERROR) {
		free(softc);
		self->softc = NULL;
		return -1;
	}

	return 0;
}

static void macfb_detach(struct device* self) {
	struct macfb_softc* softc = self->softc;

	if(softc == NULL) return;

	ttydev_detach(self);

	free(softc);
	self->softc = NULL;
}

struct device_driver macfb_driver = {
    "macfb",
    macfb_match,
    macfb_attach,
    NULL,
    macfb_detach};
