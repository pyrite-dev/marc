#include <sys/syscall.h>

void _syscall_args(struct iregs* iregs, struct syscall_args* args) {
	args->number = iregs->d[0];
	args->arg1   = iregs->d[1];
	args->arg2   = iregs->d[2];
	args->arg3   = iregs->d[3];
	args->arg4   = iregs->d[4];
	args->arg5   = iregs->d[5];
}

void _syscall_return(struct iregs* iregs, reg_t v) {
	iregs->d[0] = v;
}
