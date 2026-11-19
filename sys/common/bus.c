#include <sys/bus.h>
#include <stdlib.h>

/* TODO: These functions do not care about which byte to pick when stride is not default */
/* TODO: Add I/O */

void bus_space_init(bus_space_t* space, paddr_t locators[DEVICE_LOCATOR_COUNT]) {
	space->port   = locators[DEVICE_LOCATOR_PORT];
	space->addr   = locators[DEVICE_LOCATOR_ADDR];
	space->stride = locators[DEVICE_LOCATOR_STRIDE];
}

static int bus_space_stride(bus_space_t* space, int def) {
	return space->stride == DEVICE_LOCATOR_UNSET ? def : space->stride;
}

unsigned char bus_space_read8(bus_space_t* space, paddr_t offset) {
	int stride = bus_space_stride(space, 1);

	if(stride <= 1) {
		volatile unsigned char* ptr = (volatile unsigned char*)(space->addr + offset * stride);

		return *ptr;
	} else if(stride == 2) {
		return bus_space_read16(space, offset);
	} else if(stride == 4) {
		return bus_space_read32(space, offset);
	} else {
		volatile unsigned char* ptr = (volatile unsigned char*)(space->addr + offset * stride);

		return *ptr;
	}
}

void bus_space_write8(bus_space_t* space, paddr_t offset, unsigned char value) {
	int stride = bus_space_stride(space, 1);

	if(stride <= 1) {
		volatile unsigned char* ptr = (volatile unsigned char*)(space->addr + offset * stride);

		*ptr = value;
	} else if(stride == 2) {
		bus_space_write16(space, offset, value);
	} else if(stride == 4) {
		bus_space_write32(space, offset, value);
	} else {
		volatile unsigned char* ptr = (volatile unsigned char*)(space->addr + offset * stride);

		*ptr = value;
	}
}

unsigned short bus_space_read16(bus_space_t* space, paddr_t offset) {
	int stride = bus_space_stride(space, 2);

	if(stride == 4) {
		return bus_space_read32(space, offset);
	} else {
		volatile unsigned short* ptr = (volatile unsigned short*)(space->addr + offset * stride);

		return *ptr;
	}
}

void bus_space_write16(bus_space_t* space, paddr_t offset, unsigned short value) {
	int stride = bus_space_stride(space, 2);

	if(stride == 4) {
		bus_space_write32(space, offset, value);
	} else {
		volatile unsigned short* ptr = (volatile unsigned short*)(space->addr + offset * stride);

		*ptr = value;
	}
}

unsigned int bus_space_read32(bus_space_t* space, paddr_t offset) {
	int stride = bus_space_stride(space, 4);

	volatile unsigned int* ptr = (volatile unsigned int*)(space->addr + offset * stride);

	return *ptr;
}

void bus_space_write32(bus_space_t* space, paddr_t offset, unsigned int value) {
	int stride = bus_space_stride(space, 4);

	volatile unsigned int* ptr = (volatile unsigned int*)(space->addr + offset * stride);

	*ptr = value;
}
