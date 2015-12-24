#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

volatile int tmp = 0;

int main(void) {
	int i = 0;
	__sync_val_compare_and_swap(&i, 0, 5);
	tmp = 2;
	__sync_val_compare_and_swap(&i, 0, 7);
	tmp = 3;
	printf("%d, %d\n", i, tmp);
	return 0;
}
