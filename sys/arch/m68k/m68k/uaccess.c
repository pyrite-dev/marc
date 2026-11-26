#include <sys/uaccess.h>
#include <sys/errno.h>
#include <string.h>

int copyin(void* kaddr, const void* uaddr, size_t len) {
	memcpy(kaddr, uaddr, len);
	return 0;
}

int copyout(void* uaddr, const void* kaddr, size_t len) {
	memcpy(uaddr, kaddr, len);
	return 0;
}

int copyinstr(void* kaddr, const void* uaddr, size_t maxlen, size_t* copied) {
	char*	    dst = (char*)kaddr;
	const char* src = (const char*)uaddr;
	size_t	    i;

	for(i = 0; i < maxlen; i++) {
		dst[i] = src[i];

		if(dst[i] == '\0') {
			if(copied != NULL) *copied = i + 1;
			return 0;
		}
	}

	if(copied != NULL) *copied = maxlen;

	return ENAMETOOLONG;
}
