#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ppu_thread.h>
#include <sys/sys_time.h>
#include <cell/atomic.h>

int len = 5000;
int a = 1, b = 1;
uint32_t* arr;

void thread_entry_asc(uint64_t arg) {
	for (int x = 0; x < 150; ++x) {
		for (int i = 0; i < len; i += 2) {
			arr[i] = ++a;
		}
	}
	sys_ppu_thread_exit(0);
}

void thread_entry_desc(uint64_t arg) {
	for (int x = 0; x < 150; ++x) {
		for (int i = len - 1; i > 0; i -= 2) {
			arr[i] = ++b;
		}
	}
	sys_ppu_thread_exit(0);
}

struct S {
	uint32_t a;
	uint32_t b;
};

S __attribute__((aligned(128))) loc = {0};

int main() {

	printf("loc address %x\n", &loc);
	printf("cache line %x\n", (uint32_t)&loc & 0xffffff80);

	uint32_t val;

	printf("simple incrementing\n");

    do {
        val = __lwarx(&loc.a);
		val++;
    } while(__stwcx(&loc.a, val) == 0);

	printf("loc = {%d, %d}\n", loc.a, loc.b);

	printf("incrementing with modification of unrelated location on the same granule\n");

	do {
        val = __lwarx(&loc.a);
		val++;
		loc.b++;
    } while(__stwcx(&loc.a, val) == 0);

	printf("loc = {%d, undefined} is b > 0: %d\n", loc.a, loc.b > 0);

	loc.a = 0; loc.b = 0;

	printf("creating reservation for a, but storing to b\n");

	do {
        val = __lwarx(&loc.a);
    } while(__stwcx(&loc.b, 7) == 0);

	printf("loc = {%d, %d}\n", loc.a, loc.b);

	printf("creating reservation for a, then doing regular store to a\n");

	do {
        val = __lwarx(&loc.a);
		loc.a++;
    } while(__stwcx(&loc.a, 10) == 0);

	printf("loc = {%d, %d}\n", loc.a, loc.b);
}