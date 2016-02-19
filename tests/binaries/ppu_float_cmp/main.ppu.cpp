#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cell/gcm.h>
using namespace cell::Gcm;

#define HOST_SIZE (1024*1024)
#define COMMAND_SIZE (65536)

volatile float f;

int main() {
	f = 1.0f;
	printf("%d ", f <= 1.0f);
	printf("%d ", f >= 0.0f);
	printf("%d ", f == 1.0f);
	printf("%d ", f != 1.0f);
}