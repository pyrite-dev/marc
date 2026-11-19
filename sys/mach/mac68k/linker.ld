OUTPUT_FORMAT(elf32-m68k)
OUTPUT_ARCH(m68k)
ENTRY(_start)

SECTIONS
{
	. = 0x2000 + SIZEOF_HEADERS;

	.text : {
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4);
	}

	.data : {
		. = ALIGN(4);
		_data_start = .;
		*(.data)
		*(.data.*)
		. = ALIGN(4);
		_data_end = .;
	}
	_data_load = LOADADDR(.data);

	.bss : {
		. = ALIGN(4);
		_bss_start = .;
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		_bss_end = .;
	}

	.stack (NOLOAD) : {
		. = ALIGN(4);
		_kernel_stack_bottom = .;
		. += 4096;
		_kernel_stack_top = .;
	}

	_end = .;

	/DISCARD/ : {
		*(.comment)
		*(.note*)
	}
}
