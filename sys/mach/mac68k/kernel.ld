OUTPUT_FORMAT(elf32-m68k)
OUTPUT_ARCH(m68k)
ENTRY(_start)

MEMORY
{
	RAM (rwx) : ORIGIN = 0x2000, LENGTH = 0x400000 - 0x2000
}

SECTIONS
{
	.text : {
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4);
	} > RAM

	.data : {
		. = ALIGN(4);
		_data_start = .;
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		_data_end = .;
	} > RAM
	_data_load = LOADADDR(.data);

	.bss : {
		. = ALIGN(4);
		_bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		_bss_end = .;
	} > RAM

	.stack (NOLOAD) : {
		. = ALIGN(4);
		_kernel_stack_bottom = .;
		. += 4096;
		_kernel_stack_top = .;
	} > RAM

	_end = .;

	/DISCARD/ : {
		*(.comment)
		*(.note*)
	}
}
