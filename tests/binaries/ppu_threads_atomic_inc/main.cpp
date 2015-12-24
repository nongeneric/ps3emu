#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

uint64_t __thread tls_int = 0;
uint8_t __thread huge_tls_block[1024 * 256] = { 1, 2, 3 };

void thread_entry(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 10000; ++n) {
		__sync_fetch_and_add(i, 2);
	}
	sys_ppu_thread_exit(1);
}

int main(void) {
	int i = 0;

	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, thread_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, thread_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, thread_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, thread_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	printf("exitstatus: %d; i: %d\n", exitstatus, i);

	return 0;
}
