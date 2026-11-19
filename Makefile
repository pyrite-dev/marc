.PHONY: all depend format clean floppy

MACH = micro960
ARCH = i960
CONFIG = GENERIC
ifeq ($(MACH), mac68k)
FLOPPY_IMAGE_SIZE = 800K
else
FLOPPY_IMAGE_SIZE = 1440K
endif
TOP = .

INHERIT = MACH=$(MACH) ARCH=$(ARCH) CONFIG=$(CONFIG) FLOPPY_IMAGE_SIZE=$(FLOPPY_IMAGE_SIZE) TOP=..

all: world

world: compile
	cd compile && $(INHERIT) $(MAKE)

compile:
	$(INHERIT) $(MAKE) depend

depend:
	mkdir -p compile
	mkdir -p compile/sys
	mkdir -p compile/lib/libc
	mkdir -p compile/floppy
	./tools/config.pl sys/mach/$(MACH)/conf/$(CONFIG)
	$(INHERIT) ./tools/mkmake.sh all > compile/Makefile
	$(INHERIT) ./tools/mkmake.sh kernel > compile/Makefile.kernel
	$(INHERIT) ./tools/mkmake.sh libc > compile/Makefile.libc
	$(INHERIT) ./tools/mkmake.sh floppy > compile/Makefile.floppy

format:
	clang-format --verbose -i `find sys lib include -name "*.c" -or -name "*.h"`

ifeq ($(MACH), mac68k)
floppy: world
	cd compile && $(INHERIT) $(MAKE) -f Makefile.floppy
endif

clean:
	rm -rf compile
