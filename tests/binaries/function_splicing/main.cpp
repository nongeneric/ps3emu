#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

int single_bb(int a, int b) {
	return a + b;
}

int multiple_bb(int a, int b) {
	printf("multiple_bb(%d, %d) first\n", a, b);
	printf("multiple_bb(%d, %d) second\n", a, b);
	return a + b;
}

int multiple_recursive_bb(int a, int b) {
	int res = single_bb(b, a);
	return multiple_bb(2 * a, 3 * b);
}

uint32_t opcode;

int main(void) {
	uint32_t NCALL_OPCODE = 1;
	opcode = (NCALL_OPCODE << 26) | 193;
	uint32_t descr[2];
	memcpy(descr, (void*)&single_bb, 8);
	descr[0] = (uint32_t)&opcode;

	int (*fn)(void*,void*,void*) = (int (*)(void*,void*,void*))descr;

	printf("main\n");
	fn((void*)&single_bb, (void*)&multiple_bb, (void*)&multiple_recursive_bb);
	
	printf("---------\n");
	single_bb(1, 2);
	printf("---------\n");
	multiple_bb(5, 6);
	printf("---------\n");
	multiple_recursive_bb(7, 9);
	printf("---------\n");

	printf("done\n");

	return 0;
}
