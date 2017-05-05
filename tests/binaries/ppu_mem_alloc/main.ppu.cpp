#include <sys/memory.h>
#include <stdint.h>
#include <stdio.h>

int main() {
	sys_addr_t host_addr;
	uint32_t allocated = 0;
	for (;;) {
		int res = sys_memory_allocate(0x100000, SYS_MEMORY_PAGE_SIZE_1M, &host_addr);
		if (res == ENOMEM)
			break;
		allocated += 0x100000;
		if (allocated == 0x100000 * 180)
			printf("at lease 180 mb have been allocated\n");
	}
}