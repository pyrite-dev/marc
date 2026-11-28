#include <sys/bus.h>
#include <stdlib.h>

/* TODO: Add I/O */

void bus_space_init(bus_space_t* space, paddr_t locators[DEVICE_LOCATOR_COUNT]) {
	space->port   = locators[DEVICE_LOCATOR_PORT];
	space->addr   = locators[DEVICE_LOCATOR_ADDR];
	space->stride = locators[DEVICE_LOCATOR_STRIDE];
}

static int bus_space_stride(bus_space_t* space, int def) {
	return space->stride == DEVICE_LOCATOR_UNSET ? def : space->stride;
}

static paddr_t bus_space_addr(bus_space_t* space, paddr_t offset, int def) {
	int	stride = bus_space_stride(space, def);
	paddr_t addr   = space->addr;

	if(space->stride != DEVICE_LOCATOR_UNSET) addr -= addr % stride;

	return addr + offset * stride;
}

static int bus_space_index(bus_space_t* space, int def) {
	int stride = bus_space_stride(space, def);

	if(space->stride == DEVICE_LOCATOR_UNSET) return 0;

	return (space->addr % stride) / def;
}

unsigned char bus_space_read8(bus_space_t* space, paddr_t offset) {
	volatile unsigned char* ptr = (volatile unsigned char*)bus_space_addr(space, offset, 1);

	return ptr[bus_space_index(space, 1)];
}

void bus_space_write8(bus_space_t* space, paddr_t offset, unsigned char value) {
	volatile unsigned char* ptr = (volatile unsigned char*)bus_space_addr(space, offset, 1);

	ptr[bus_space_index(space, 1)] = value;
}

unsigned short bus_space_read16(bus_space_t* space, paddr_t offset) {
	volatile unsigned short* ptr = (volatile unsigned short*)bus_space_addr(space, offset, 2);

	return ptr[bus_space_index(space, 2)];
}

void bus_space_write16(bus_space_t* space, paddr_t offset, unsigned short value) {
	volatile unsigned short* ptr = (volatile unsigned short*)bus_space_addr(space, offset, 2);

	ptr[bus_space_index(space, 2)] = value;
}

unsigned int bus_space_read32(bus_space_t* space, paddr_t offset) {
	volatile unsigned int* ptr = (volatile unsigned int*)bus_space_addr(space, offset, 4);

	return ptr[bus_space_index(space, 4)];
}

void bus_space_write32(bus_space_t* space, paddr_t offset, unsigned int value) {
	volatile unsigned int* ptr = (volatile unsigned int*)bus_space_addr(space, offset, 4);

	ptr[bus_space_index(space, 4)] = value;
}
