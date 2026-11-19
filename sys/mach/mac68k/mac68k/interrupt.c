#include <sys/interrupt.h>

int _intr_vector(unsigned int irq) {
	if(irq == 1) return 25; /* VIA */
	if(irq == 2) return 26; /* SCC */

	return -1;
}
