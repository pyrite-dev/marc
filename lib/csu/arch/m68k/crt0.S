	.text

/* copies .data from its load address to the address it was linked at */
copy_data:
	lea	_data_load,%a0
	lea	_data_start,%a1
	lea	_data_end,%a2

	cmpal	%a1,%a0
	beq	2f /* already in place, nothing to copy */

1:	cmpal	%a2,%a1
	bcc	2f
	movel	(%a0)+,(%a1)+
	bra	1b
2:	rts

/* clears .bss */
clear_bss:
	lea	_bss_start,%a0
	lea	_bss_end,%a1

1:	cmpal	%a1,%a0
	bcc	2f
	clrl	(%a0)+
	bra	1b
2:	rts

	.globl	_start
_start:
#ifdef __KERNEL__
	movew	#0x2700,%sr

	/* the bootloader's stack is not used past this point, save the arguments */
	movel	4(%sp),%d0
	movel	8(%sp),%d1
	movel	12(%sp),%d2

	lea	_kernel_stack_top,%sp
#endif

	jsr	copy_data
	jsr	clear_bss

#ifdef __KERNEL__
	movel	%d0,boot_screen_width
	movel	%d1,boot_screen_height
	movel	%d2,boot_cmdline

	jsr	kmain
1:	bra	1b
#else
	jsr	_main
#endif
