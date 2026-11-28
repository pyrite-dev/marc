#include <sys/cons.h>
#include <sys/machdep.h>
#include <sys/interrupt.h>
#include <sys/osinfo.h>
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <sys/syscall.h>
#include <sys/vfs.h>

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

static thread_t thread;
static char	stack[4096];

static void thread_main(void* arg) {
	const char*   text = "Hello, Console!\n";
	int	      fd   = open("/dev/tty0", O_RDWR);
	int	      infd = open("/dev/tty1", O_RDWR);
	DIR*	      dir  = opendir("/dev");
	int	      d	   = open("/dev/wd0a", O_RDONLY);
	unsigned char buf[512];
	int	      i;

	if(infd < 0) infd = fd;

	write(fd, text, strlen(text));

	if(d >= 0) {
		lseek(d, 512, SEEK_SET);
		read(d, buf, 512);
		for(i = 0; i < 8; i += 4) {
			if(i > 0 && (i % 16) == 0) write(fd, "\n", 1);
			dprintf(fd, "%02x %02x %02x %02x ", (unsigned int)buf[i + 0], (unsigned int)buf[i + 1], (unsigned int)buf[i + 2], (unsigned int)buf[i + 3]);
		}
		write(fd, "\n", 1);
		close(d);
	}

	if(dir != NULL) {
		struct dirent* d;

		while((d = readdir(dir)) != NULL) {
			write(fd, d->d_name, strlen(d->d_name));
			write(fd, "\n", 1);
		}
	}

	while(1) {
		char c;

		read(infd, &c, 1);
		write(fd, &c, 1);
	}
}

void kmain(void) {
	intr_flags_t flags;

	machdep_init();
	intr_init();
	syscall_init();
	sched_init();
	cons_init();

	kprintf("Marc/%s %s (%s): %s\r\n", os_machine, os_version, os_ident, os_date);
	kprintf("Available memory = %ld KB\r\n", (long)kmem_avail / 1024);
	kprintf("\r\n");
	device_init();

	flags = _intr_save();
	kmem_use_spinlock();
	device_start();
	_intr_restore(flags);

	vfs_init();
	file_init();

	thread_init(&thread, stack, sizeof(stack), thread_main, NULL);
	sched_add(&thread);

	sched_start();
}
