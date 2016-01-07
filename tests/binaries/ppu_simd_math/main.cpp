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

void soa_tests();
void aos_tests();

int main(void) {
	soa_tests();
	aos_tests();
	return 0;
}
