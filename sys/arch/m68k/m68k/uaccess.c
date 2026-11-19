#include <sys/uaccess.h>
#include <string.h>

int copyin(void* kaddr, const void* uaddr, size_t len) {
	memcpy(kaddr, uaddr, len);
	return 0;
}

int copyout(void* uaddr, const void* kaddr, size_t len) {
	memcpy(uaddr, kaddr, len);
	return 0;
}
