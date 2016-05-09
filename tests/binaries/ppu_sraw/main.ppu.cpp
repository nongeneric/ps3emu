#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int a = 1000;
volatile int s = 0;
volatile int one = 1;

int main() {
	volatile int r = a >> s;
	printf("%d\n", r);
	volatile int r2 = a << s;
	printf("%d\n", r2);
	volatile int r3 = a >> 2;
	printf("%d\n", r3);
	volatile int r4 = a << 2;
	printf("%d\n", r4);
	volatile int r5 = a >> one;
	printf("%d\n", r5);
}