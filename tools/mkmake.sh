#!/bin/sh
MATCH='-name "*.c" -or -name "*.s" -or -name "*.S"'

if [ "$1" = "all" ]; then
	echo ".PHONY: all kernel clean"
	echo ""
	echo "all: kernel libc"
	echo ""
	echo "kernel: libc"
	echo "	\$(MAKE) -f Makefile.kernel"
	echo ""
	echo "libc:"
	echo "	\$(MAKE) -f Makefile.libc"
	echo ""
	echo "floppy: kernel"
	echo "	\$(MAKE) -f Makefile.floppy"
	echo ""
	echo "clean:"
	echo "	\$(MAKE) -f Makefile.kernel clean"
	echo "	\$(MAKE) -f Makefile.libc clean"
elif [ "$1" = "kernel" ]; then
	./tools/prelude.sh sys "\$(KERNEL)"
	(
		eval find \
			sys/mach/$MACH/$MACH \
			sys/arch/$ARCH/$ARCH \
			sys/common \
			-maxdepth 1 $MATCH
		ls -d lib/csu/arch/$ARCH/crt0.*
		ls -d compile/sys/*.c
	) | ./tools/makedep.sh sys
	cat compile/files.kernel
	echo "OBJS += \$(TOP)/compile/lib/libc.a"
	echo ""
	cat sys/mach/$MACH/Makefile.in
	cat sys/Makefile.in
elif [ "$1" = "libc" ]; then
	./tools/prelude.sh lib/libc "lib/libc.a"
	(
		LIBC="`ls -d lib/libc/* | grep -v Makefile | grep -v arch`"

		eval find \
			$LIBC \
			lib/libc/arch/$ARCH \
			$MATCH
	) | ./tools/makedep.sh lib/libc
	cat lib/libc/Makefile.in
elif [ "$1" = "floppy" ]; then
    if [ "$MACH" = "mac68k" ]; then
		./tools/prelude.sh external/macboot "\$(FLOPPY)"
		(
			eval find \
				external/macboot \
				-maxdepth 1 -name "*.s"
		) | ./tools/makedep.sh floppy
		echo "floppy/boot_block.bin: \$(OBJS)"
		echo "	\$(LD) -T \$(TOP)/external/macboot/boot_block.ld -o \$@ \$<"
		echo ""
		echo "\$(FLOPPY): floppy/boot_block.bin"
		echo "	-rm floppy/fs.img"
		echo "	mkdir -p floppy/fs"
		echo "	cp \$(KERNEL) floppy/fs/kernel"
		echo "	touch floppy/fs/cmdline"
		echo "	mke2fs -t ext2 -d floppy/fs floppy/fs.img \$(FLOPPY_IMAGE_SIZE)"
		echo "	head -c 1024 \$< > \$@"
		echo "	tail -c +1025 floppy/fs.img >> \$@"
	fi
fi
