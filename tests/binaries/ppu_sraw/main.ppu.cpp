#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile unsigned a = 1000;
volatile unsigned s = 0;

int main() {
	volatile unsigned r = a >> s;
	printf("%d\n", r);
	volatile unsigned r2 = a << s;
	printf("%d\n", r2);
	volatile unsigned r3 = a >> 2;
	printf("%d\n", r3);
	volatile unsigned r4 = a << 2;
	printf("%d\n", r4);
}