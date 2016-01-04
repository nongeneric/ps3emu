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

SYS_PROCESS_PARAM(1001, 0x10000)

static int static_var = 0;
int __thread tls_var = 0;

void thread_entry(uint64_t arg) {
	int* i = (int*)arg;
	int flag = 0;
	if (sys_process_is_stack(i))
		flag |= 0x0100;
	if (sys_process_is_stack(&static_var))
		flag |= 0x0010;
	if (sys_process_is_stack(&tls_var))
		flag |= 0x0001;
	if (sys_process_is_stack(&flag))
		flag |= 0x1000;
	sys_ppu_thread_exit(flag);
}

int main(void) {
	int i = 0;
	
	sys_ppu_thread_t id1;
	sys_ppu_thread_create(&id1, thread_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);

	int flag = 0;
	if (sys_process_is_stack(&i))
		flag |= 0x100;
	if (sys_process_is_stack(&static_var))
		flag |= 0x010;
	if (sys_process_is_stack(&tls_var))
		flag |= 0x001;
	if (sys_process_is_stack(&i + 0x20000))
		flag |= 0x1000;

	printf("main thread: %x\n", flag);
	printf("other thread: %x\n", exitstatus);

	return 0;
}
