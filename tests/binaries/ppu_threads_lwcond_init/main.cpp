#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

int main(void) {
	sys_lwmutex_t mutex;
	sys_lwcond_t cv;

	sys_lwmutex_attribute_t attr;
	sys_lwmutex_attribute_initialize(attr);
	sys_lwmutex_create(&mutex, &attr);

	printf("sys_lwmutex_t.recursive_count %x\n", mutex.recursive_count);
	printf("sys_lwmutex_t.attribute %x\n", mutex.attribute);
	printf("sys_lwmutex_t.lock_var.all_info %x\n", mutex.lock_var.all_info);
	printf("SYS_SYNC_PRIORITY | SYS_SYNC_NOT_RECURSIVE %x\n", SYS_SYNC_PRIORITY | SYS_SYNC_NOT_RECURSIVE);

	sys_lwcond_attribute_t cv_attr;
	sys_lwcond_attribute_initialize(cv_attr);
	sys_lwcond_create(&cv, &mutex, &cv_attr);

	printf("&mutex == cv.lwmutex %d\n", &mutex == cv.lwmutex);

	return 0;
}
