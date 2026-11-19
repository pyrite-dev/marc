#ifndef __MACHINE_REGS_H__
#define __MACHINE_REGS_H__

struct iregs {
	unsigned int d[8];
	unsigned int a[7];
	unsigned int sp;
	unsigned int sr;
	unsigned int pc;
};

#endif
