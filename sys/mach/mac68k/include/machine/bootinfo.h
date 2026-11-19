#ifndef __MACHINE_BOOTINFO_H__
#define __MACHINE_BOOTINFO_H__

#include <stdint.h>

#include <sys/cdefs.h>

/* the command line is copied out of the boot block's buffers before the heap uses them */
#define BOOT_ARGS_MAX 128

__BEGIN_EXTERN_C__

extern uint32_t boot_screen_width;
extern uint32_t boot_screen_height;
extern char	boot_args[BOOT_ARGS_MAX];

__END_EXTERN_C__

#endif
