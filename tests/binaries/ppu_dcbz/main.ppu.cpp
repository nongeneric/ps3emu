#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cell/gcm.h>
using namespace cell::Gcm;

#define HOST_SIZE (1024*1024)
#define COMMAND_SIZE (65536)

void dcbz(void* dst) {
	__asm__ volatile(
		"dcbz 0,%0;\t\n"
		:: "r"(dst) );
}

void printhex(void* buf, int size) {
	uint8_t* ptr = (uint8_t*)buf;
	for (int i = 0; i < size; ++i) {
		printf("%x", ptr[i]);
		if ((i + 1) % 64 == 0)
			printf("\n");
	}
	printf("\n");
}

int main() {
	void* buf = memalign(128, 1024 * 4);
	
	memset(buf, 1, 1024 * 4);
	dcbz(buf);
	printhex(buf, 256);

	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 1);
	printhex(buf, 256);

	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 16);
	printhex(buf, 256);

	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 32);
	printhex(buf, 256);

	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 64);
	printhex(buf, 256);

	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 66);
	printhex(buf, 256);
	
	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 128);
	printhex(buf, 256);
	
	memset(buf, 1, 1024 * 4);
	dcbz((uint8_t*)buf + 129);
	printhex(buf, 256);
}