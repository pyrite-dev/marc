#include <sys/tty.h>
#include <sys/ttydev.h>
#include <sys/termios.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>

#include <string.h>

static int ttyq_put(struct tty_queue* q, unsigned char data) {
	if(q->count >= TTYQ_SIZE) return 0;

	q->data[q->tail] = data;
	q->tail		 = (q->tail + 1) % TTYQ_SIZE;
	q->count++;

	return 1;
}

static int ttyq_get(struct tty_queue* q, unsigned char* data) {
	if(q->count == 0) return 0;

	*data	= q->data[q->head];
	q->head = (q->head + 1) % TTYQ_SIZE;
	q->count--;

	return 1;
}

static int ttyq_peek(struct tty_queue* q, unsigned char* data) {
	if(q->count == 0) return 0;

	*data = q->data[q->head];

	return 1;
}

static int ttyq_unput(struct tty_queue* q) {
	if(q->count == 0) return 0;

	q->tail = (q->tail + TTYQ_SIZE - 1) % TTYQ_SIZE;
	q->count--;

	return 1;
}

static void ttyq_clear(struct tty_queue* q) {
	q->head	 = 0;
	q->tail	 = 0;
	q->count = 0;
}

static unsigned int ttyq_space(struct tty_queue* q) {
	return TTYQ_SIZE - q->count;
}

static int ttyrec_put(struct tty_canrec_queue* q, unsigned int length) {
	if(q->count >= TTYQ_SIZE) return 0;

	q->data[q->tail] = length;
	q->tail		 = (q->tail + 1) % TTYQ_SIZE;
	q->count++;

	return 1;
}

static void ttyrec_consume(struct tty_canrec_queue* q) {
	if(q->count == 0) return;

	if(--q->data[q->head] == 0) {
		q->head = (q->head + 1) % TTYQ_SIZE;
		q->count--;
	}
}

static void ttyrec_discard(struct tty_canrec_queue* q) {
	if(q->count == 0) return;

	q->head = (q->head + 1) % TTYQ_SIZE;
	q->count--;
}

static int tty_commit_raw(struct tty* tty, int eof) {
	unsigned int  length;
	unsigned char c;

	length = tty->rawq.count;

	if(length == 0) {
		if(eof) return ttyrec_put(&tty->canrecq, 0);
		return 0;
	}

	if(ttyq_space(&tty->canq) < length || tty->canrecq.count >= TTYQ_SIZE) {
		return 0;
	}

	while(ttyq_get(&tty->rawq, &c)) ttyq_put(&tty->canq, c);

	ttyrec_put(&tty->canrecq, length);
	return 1;
}

void tty_init(struct tty* tty, const struct tty_ops* ops) {
	memset(tty, 0, sizeof(*tty));

	spinlock_init(&tty->lock);

	waiter_init(&tty->rwait);
	waiter_init(&tty->wwait);

	mutex_init(&tty->read_mutex);
	mutex_init(&tty->write_mutex);
	mutex_init(&tty->param_mutex);

	tty->ops = ops;

	tty->termios.c_iflag = ICRNL;
	tty->termios.c_oflag = OPOST | ONLCR;
	tty->termios.c_cflag = CREAD | CS8;
	tty->termios.c_lflag = ICANON | ECHO | ECHOE;

	tty->termios.c_cc[VERASE] = 0x7f;
	tty->termios.c_cc[VKILL]  = 0x15;
	tty->termios.c_cc[VEOF]	  = 0x04;
	tty->termios.c_cc[VSTART] = 0x11;
	tty->termios.c_cc[VSTOP]  = 0x13;
	tty->termios.c_cc[VMIN]	  = 1;
	tty->termios.c_cc[VTIME]  = 0;
}

int tty_open(struct tty* tty, int flags, int mode) {
	intr_flags_t irq;

	irq = spinlock_lock_irqsave(&tty->lock);

	if(tty->open_count == 0) tty->state |= TS_ISOPEN;

	tty->open_count++;
	spinlock_unlock_irqrestore(&tty->lock, irq);

	return 0;
}

int tty_close(struct tty* tty) {
	intr_flags_t irq;

	irq = spinlock_lock_irqsave(&tty->lock);

	if(tty->open_count != 0) {
		tty->open_count--;

		if(tty->open_count == 0) tty->state &= ~TS_ISOPEN;
	}

	spinlock_unlock_irqrestore(&tty->lock, irq);

	return 0;
}

int tty_read(struct tty* tty, struct uio* uio, int flags) {
	intr_flags_t	  irq;
	struct tty_queue* q;
	unsigned char	  c;
	size_t		  initial, limit, minimum;
	unsigned int	  record_length;
	int		  st, canonical, vmin, vtime;

	st	= 0;
	initial = uio->uio_resid;

	mutex_lock(&tty->read_mutex);

	while(uio->uio_resid != 0) {
		irq = spinlock_lock_irqsave(&tty->lock);

		canonical = (tty->termios.c_lflag & ICANON) != 0;

		if(canonical) {
			q	= &tty->canq;
			minimum = 0;
		} else {
			q = &tty->rawq;

			vmin  = tty->termios.c_cc[VMIN];
			vtime = tty->termios.c_cc[VTIME];

			if(vtime != 0) {
				spinlock_unlock_irqrestore(&tty->lock, irq);
				st = EOPNOTSUPP;
				break;
			}

			minimum = (size_t)vmin;
			if(minimum > uio->uio_resid) minimum = uio->uio_resid;
		}

		while(canonical ? tty->canrecq.count == 0 : q->count < minimum) {
			if((flags & O_NONBLOCK) && (canonical ? tty->canrecq.count == 0 : q->count == 0)) {
				spinlock_unlock_irqrestore(&tty->lock, irq);
				st = EWOULDBLOCK;
				goto done;
			}

			if((flags & O_NONBLOCK) || (!canonical && minimum == 0)) break;

			waiter_sleep(&tty->rwait, &tty->lock, irq);

			canonical = (tty->termios.c_lflag & ICANON) != 0;
			q	  = canonical ? &tty->canq : &tty->rawq;

			if(!canonical) {
				vmin  = tty->termios.c_cc[VMIN];
				vtime = tty->termios.c_cc[VTIME];

				if(vtime != 0) {
					spinlock_unlock_irqrestore(&tty->lock, irq);
					st = EOPNOTSUPP;
					goto done;
				}

				minimum = (size_t)vmin;
				if(minimum > uio->uio_resid) minimum = uio->uio_resid;
			} else {
				minimum = 0;
			}
		}

		if(canonical) {
			record_length = tty->canrecq.data[tty->canrecq.head];

			if(record_length == 0) {
				ttyrec_discard(&tty->canrecq);
				spinlock_unlock_irqrestore(&tty->lock, irq);
				break;
			}

			limit = (size_t)record_length;
		} else {
			limit = q->count;
		}

		if(limit > uio->uio_resid) limit = uio->uio_resid;

		spinlock_unlock_irqrestore(&tty->lock, irq);

		while(limit != 0 && uio->uio_resid != 0) {
			irq = spinlock_lock_irqsave(&tty->lock);

			if(!ttyq_peek(q, &c)) {
				spinlock_unlock_irqrestore(&tty->lock, irq);
				break;
			}

			spinlock_unlock_irqrestore(&tty->lock, irq);

			st = uiomove(&c, 1, uio);
			if(st != 0) goto done;

			irq = spinlock_lock_irqsave(&tty->lock);

			if(ttyq_get(q, &c) && canonical) ttyrec_consume(&tty->canrecq);

			spinlock_unlock_irqrestore(&tty->lock, irq);

			limit--;
		}

		if(canonical) break;

		break;
	}

done:
	mutex_unlock(&tty->read_mutex);

	if(st != 0 && initial != uio->uio_resid) st = 0;

	return st;
}

int tty_write(struct tty* tty, struct uio* uio, int flags) {
	intr_flags_t  irq;
	unsigned char c;
	unsigned int  oflag;
	size_t	      initial;
	int	      st;

	st	= 0;
	initial = uio->uio_resid;

	mutex_lock(&tty->write_mutex);

	while(uio->uio_resid != 0) {
		irq = spinlock_lock_irqsave(&tty->lock);

		while(ttyq_space(&tty->outq) < tty->out_reserved + 2) {
			if(flags & O_NONBLOCK) {
				spinlock_unlock_irqrestore(&tty->lock, irq);
				st = EWOULDBLOCK;
				goto done;
			}

			waiter_sleep(&tty->wwait, &tty->lock, irq);
		}

		tty->out_reserved += 2;

		spinlock_unlock_irqrestore(&tty->lock, irq);

		st = uiomove(&c, 1, uio);

		irq = spinlock_lock_irqsave(&tty->lock);

		tty->out_reserved -= 2;

		if(st == 0) {
			oflag = tty->termios.c_oflag;

			if((oflag & OPOST) && (oflag & ONLCR) && c == '\n') {
				ttyq_put(&tty->outq, '\r');
				ttyq_put(&tty->outq, '\n');
			} else {
				ttyq_put(&tty->outq, c);
			}
		}

		spinlock_unlock_irqrestore(&tty->lock, irq);

		if(st != 0) break;

		tty_start(tty);
	}

done:
	mutex_unlock(&tty->write_mutex);

	if(st != 0 && initial != uio->uio_resid) st = 0;

	return st;
}

static void tty_drain(struct tty* tty) {
	intr_flags_t irq;

	mutex_lock(&tty->write_mutex);

	irq = spinlock_lock_irqsave(&tty->lock);

	while(tty->outq.count != 0) waiter_sleep(&tty->wwait, &tty->lock, irq);

	spinlock_unlock_irqrestore(&tty->lock, irq);

	mutex_unlock(&tty->write_mutex);
}

int tty_ioctl(struct tty* tty, unsigned long cmd, void* data, int flags) {
	intr_flags_t irq;

	switch(cmd) {
	case TIOCGETA:
		irq		       = spinlock_lock_irqsave(&tty->lock);
		*(struct termios*)data = tty->termios;
		spinlock_unlock_irqrestore(&tty->lock, irq);
		return 0;

	case TIOCSETA:
		return tty_param(tty, (const struct termios*)data);

	case TIOCSETAW:
		tty_drain(tty);
		return tty_param(tty, (const struct termios*)data);

	case TIOCSETAF:
		tty_flush(tty, TTY_FLUSH_INPUT);
		return tty_param(tty, (const struct termios*)data);
	}

	return ENOTTY;
}

void tty_start(struct tty* tty) {
	intr_flags_t	      irq;
	const struct tty_ops* ops;
	int		      start;

	irq = spinlock_lock_irqsave(&tty->lock);

	start = (tty->outq.count != 0) && !(tty->state & TS_TTSTOP);

	ops = tty->ops;

	spinlock_unlock_irqrestore(&tty->lock, irq);

	if(start && ops != NULL && ops->start != NULL) ops->start(tty);
}

static int tty_echo_locked(struct tty* tty, unsigned char c) {
	unsigned int oflag;

	oflag = tty->termios.c_oflag;

	if((oflag & OPOST) && (oflag & ONLCR) && c == '\n') {
		if(ttyq_space(&tty->outq) <= tty->out_reserved + 1) return 0;

		ttyq_put(&tty->outq, '\r');
	} else if(ttyq_space(&tty->outq) <= tty->out_reserved) {
		return 0;
	}

	return ttyq_put(&tty->outq, c);
}

void tty_input(struct tty* tty, unsigned char data, unsigned int flags) {
	intr_flags_t  irq;
	unsigned int  iflag, lflag;
	unsigned char c;
	int	      wakeup, start, canonical, delimiter;

	c      = data;
	wakeup = 0;
	start  = 0;

	irq = spinlock_lock_irqsave(&tty->lock);

	iflag	  = tty->termios.c_iflag;
	lflag	  = tty->termios.c_lflag;
	canonical = (lflag & ICANON) != 0;

	if(c == '\r') {
		if(iflag & IGNCR) {
			spinlock_unlock_irqrestore(&tty->lock, irq);
			return;
		}

		if(iflag & ICRNL) c = '\n';
	} else if(c == '\n' && (iflag & INLCR)) {
		c = '\r';
	}

	if(iflag & IXON) {
		if(c == tty->termios.c_cc[VSTOP]) {
			tty->state |= TS_TTSTOP;
			spinlock_unlock_irqrestore(&tty->lock, irq);
			return;
		}

		if(c == tty->termios.c_cc[VSTART]) {
			tty->state &= ~TS_TTSTOP;
			start = 1;
			spinlock_unlock_irqrestore(&tty->lock, irq);

			if(start) tty_start(tty);
			return;
		}
	}

	if(canonical && c == tty->termios.c_cc[VEOF] && tty->termios.c_cc[VEOF] != _POSIX_VDISABLE) {
		wakeup = tty_commit_raw(tty, 1);
		spinlock_unlock_irqrestore(&tty->lock, irq);

		if(wakeup) waiter_wakeup(&tty->rwait);
		return;
	}

	if(canonical) {
		if(c == tty->termios.c_cc[VERASE]) {
			if(ttyq_unput(&tty->rawq) && (lflag & ECHO) && (lflag & ECHOE)) {
				tty_echo_locked(tty, '\b');
				tty_echo_locked(tty, ' ');
				tty_echo_locked(tty, '\b');
				start = 1;
			}

			spinlock_unlock_irqrestore(&tty->lock, irq);

			if(start) tty_start(tty);
			return;
		}

		if(c == tty->termios.c_cc[VKILL]) {
			ttyq_clear(&tty->rawq);

			if((lflag & ECHO) && (lflag & ECHOK)) {
				tty_echo_locked(tty, '\n');
				start = 1;
			}

			spinlock_unlock_irqrestore(&tty->lock, irq);

			if(start) tty_start(tty);
			return;
		}
	}

	if(!ttyq_put(&tty->rawq, c)) {
		spinlock_unlock_irqrestore(&tty->lock, irq);
		return;
	}

	delimiter = c == '\n';

	if(canonical && delimiter) {
		wakeup = tty_commit_raw(tty, 0);
	} else if(!canonical) {
		wakeup = 1;
	}

	if(lflag & ECHO) {
		if(tty_echo_locked(tty, c)) start = 1;
	}

	spinlock_unlock_irqrestore(&tty->lock, irq);

	if(wakeup) waiter_wakeup(&tty->rwait);

	if(start) tty_start(tty);
}

int tty_output_getc(struct tty* tty, unsigned char* data) {
	intr_flags_t irq;
	int	     result;

	irq = spinlock_lock_irqsave(&tty->lock);

	if((tty->state & TS_TTSTOP) || !ttyq_get(&tty->outq, data)) {
		result = 0;
	} else {
		result = 1;
	}

	spinlock_unlock_irqrestore(&tty->lock, irq);

	if(result) waiter_wakeup(&tty->wwait);

	return result;
}

void tty_flush(struct tty* tty, int flags) {
	intr_flags_t irq;

	irq = spinlock_lock_irqsave(&tty->lock);

	if(flags & TTY_FLUSH_INPUT) {
		ttyq_clear(&tty->rawq);
		ttyq_clear(&tty->canq);
		tty->canrecq.head  = 0;
		tty->canrecq.tail  = 0;
		tty->canrecq.count = 0;
	}

	if(flags & TTY_FLUSH_OUTPUT) ttyq_clear(&tty->outq);

	spinlock_unlock_irqrestore(&tty->lock, irq);

	if(flags & TTY_FLUSH_INPUT) waiter_wakeup(&tty->rwait);

	if(flags & TTY_FLUSH_OUTPUT) waiter_wakeup(&tty->wwait);
}

int tty_param(struct tty* tty, const struct termios* termios) {
	intr_flags_t	      irq;
	struct termios	      new_termios;
	const struct tty_ops* ops;
	int		      st;

	mutex_lock(&tty->param_mutex);

	new_termios = *termios;
	ops	    = tty->ops;

	st = 0;

	if(ops != NULL && ops->param != NULL) st = ops->param(tty, &new_termios);

	if(st == 0) {
		irq	     = spinlock_lock_irqsave(&tty->lock);
		tty->termios = new_termios;
		spinlock_unlock_irqrestore(&tty->lock, irq);
	}

	mutex_unlock(&tty->param_mutex);

	return st;
}
