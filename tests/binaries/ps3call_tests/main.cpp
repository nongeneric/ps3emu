#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

int simple_cb(int a, int b) {
	printf("simple_cb(%d, %d)\n", a, b);
	return a + b;
}

int recursive_child_cb(int a) {
	printf("recursive_child_cb\n");
	return a + 10;
}

int recursive_cb(int a) {
	/*printf("recursive_cb(%d)\n", a);
	printf("calling recursive_child_cb(%d)\n", 7 - a);*/
	int r = recursive_child_cb(7 - a);
	printf("recursive child returned %d\n", r);
	return a * 2 + r;
}

uint32_t opcode;

int main(void) {
	uint32_t NCALL_OPCODE = 1;
	opcode = (NCALL_OPCODE << 26) | 192;
	uint32_t descr[2];
	memcpy(descr, (void*)&simple_cb, 8);
	descr[0] = (uint32_t)&opcode;

	int (*fn)(void*,void*,void*) = (int (*)(void*,void*,void*))descr;

	printf("main\n");
	fn((void*)&simple_cb, (void*)&recursive_cb, (void*)&recursive_child_cb);
	printf("done\n");
	return 0;
}
