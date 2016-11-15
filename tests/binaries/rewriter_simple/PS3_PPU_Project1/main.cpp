#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sysutil\sysutil_sysparam.h>
#include <sysutil\sysutil_common.h>
#include <sysutil\sysutil_syscache.h>
#include <sys/fs.h>
#include <cell/cell_fs.h>
#include <sys/paths.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>
#include <simd>

SYS_PROCESS_PARAM(1001, 0x10000)

// r - rewritten function
// i - interpreted function

// r

int test1_r(int a, int b) {
	int res = 2 * a + b / 10;
	for (int i = a; i < b; ++i) {
		res += a / 5;
	}
	return res * 2;
}

// r1 <-> r2

int test2_r2(int a);

int test2_r1(int a) {
	if (a > 1000)
		return a + 1;
	return a + 12 + test2_r2(a + 1);
}

int test2_r2(int a) {
	if (a > 600)
		return a + 2;
	return 1 + 2 * test2_r1(a + 3);
}

// r -> i

int test3_i(int a);

int test3_r(int a) {
	return test3_i(a + 170);
}

int test3_i(int a) {
	return 8 + 2 * a;
}

// r -> i -> r

int test4_i(int a);
int test4_r2(int a);

int test4_r1(int a) {
	return test4_i(a + 10);
}

int test4_i(int a) {
	return test4_r2(a * 3) * 2;
}

int test4_r2(int a) {
	return a - 20;
}

// r <-> r

int test5_r(int a) {
	if (a > 1000)
		return a * 2;
	return test5_r(a + 7);
}

// indirect call

int test6_r2(int a);
int test6_r3(int (*f)(int));

int test6_r1() {
	return test6_r3(test6_r2);
}

int test6_r2(int a) {
	return 10 + a;
}

int test6_r3(int (*f)(int)) {
	return f(7);
}

// exit

int test0_r() {
	exit(0);
}

#define FPTR(x) *(uint32_t*)x

int main(void) {
	printf("test1 ep: %x\n", FPTR(test1_r));
	printf("test1 res: %d\n", test1_r(44, 120));

	printf("test2 ep: %x %x\n", FPTR(test2_r1), FPTR(test2_r2));
	printf("test2 res: %d\n", test2_r1(0));

	printf("test3 ep: %x %x(ignore)\n", FPTR(test3_r), FPTR(test3_i));
	printf("test3 res: %d\n", test3_r(17));

	printf("test4 ep: %x %x(ignore) %x\n", FPTR(test4_r1), FPTR(test4_i), FPTR(test4_r2));
	printf("test4 res: %d\n", test4_r1(13));

	printf("test5 ep: %x\n", FPTR(test5_r));
	printf("test5 res: %d\n", test5_r(3));

	printf("test6 ep: %x\n", FPTR(test6_r1));
	printf("test6 res: %d\n", test6_r1());

	printf("test0 ep: %x\n", FPTR(test0_r));
	test0_r();

	return 0;
}
