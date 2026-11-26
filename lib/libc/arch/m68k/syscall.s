	.text

	.globl	_syscall
_syscall:
	movel	4(%sp),%d0
	movel	8(%sp),%d1
	movel	12(%sp),%d2
	movel	16(%sp),%d3
	movel	20(%sp),%d4
	movel	24(%sp),%d5
	trap	#0
	rts
