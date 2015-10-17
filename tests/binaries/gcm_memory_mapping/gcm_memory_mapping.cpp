#include <stdio.h>
#include <stdlib.h>

#include <cell/gcm.h>
using namespace cell::Gcm;

#define HOST_SIZE (1024*1024)
#define COMMAND_SIZE (65536)

int main() {
	void *host_addr = memalign(1024*1024, HOST_SIZE);
	
	printf("before gcm init");

	uint32_t offset;
	cellGcmAddressToOffset(host_addr, &offset);
	printf("host_addr to offset: %x\n", offset);
	void *addr;
	cellGcmIoOffsetToAddress(0, &addr);
	printf("offset 0 to address: %x\n", (uintptr_t)addr);

	printf("after gcm init");
	cellGcmInit( COMMAND_SIZE , HOST_SIZE , host_addr);

	cellGcmAddressToOffset(host_addr, &offset);
	printf("host_addr to offset: %x\n", offset);
	cellGcmIoOffsetToAddress(0, &addr);
	printf("offset 0 to address: %x\n", (uintptr_t)addr);

	cellGcmAddressToOffset((void*)0xc0000005, &offset);
	printf("address 0xc0000005 to offset: %x\n", offset);
}
