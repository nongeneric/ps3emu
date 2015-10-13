#include <stdio.h>
#include <stdlib.h>

#include <cell/gcm.h>
using namespace cell::Gcm;

#define HOST_SIZE (1024*1024)
#define COMMAND_SIZE (65536)

int main() {
	//Reserve a 1MB aligned chunk of memory
	void *host_addr = memalign(1024*1024, HOST_SIZE);
	//This function initializes libgcm and maps the buffer on main memory to IOaddress space so that RSX can access it.
	int err = cellGcmInit ( COMMAND_SIZE , HOST_SIZE , host_addr);
	CellGcmContextData* context = gCellGcmCurrentContext;
	printf("%x\n", context->end - context->begin);
}
