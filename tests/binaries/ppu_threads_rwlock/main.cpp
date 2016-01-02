#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_rwlock_t lock;

void test_rwlock_w_entry(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		sys_rwlock_wlock(lock, 10000);
		int tmp = *i;
		sys_timer_usleep(10);
		*i = tmp + 1;
		sys_rwlock_wunlock(lock);
	}
	sys_ppu_thread_exit(0);
}
void test_rwlock_w_entry2(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		while (sys_rwlock_trywlock(lock) != CELL_OK)
			sys_timer_usleep(10);
		(*i)++;
		sys_rwlock_wunlock(lock);
	}
	sys_ppu_thread_exit(0);
}

void test_rwlock_w() {
	int i = 0;

	sys_rwlock_attribute_t attr;
	sys_rwlock_attribute_initialize(attr);
	sys_rwlock_create(&lock, &attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_rwlock_w_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_rwlock_w_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_rwlock_w_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_rwlock_w_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	sys_rwlock_destroy(lock);

	printf("test_rwlock_w: %d; i: %d\n", exitstatus, i);
}

void test_rwlock_r_entry(uint64_t arg) {
	int* i = (int*)arg;
	sys_rwlock_rlock(lock, 0);
	sys_ppu_thread_exit(*i);
}

void test_rwlock_r() {
	int i = 10;

	sys_rwlock_attribute_t attr;
	sys_rwlock_attribute_initialize(attr);
	sys_rwlock_create(&lock, &attr);
	
	sys_rwlock_rlock(lock, 0);

	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_rwlock_r_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_rwlock_r_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_rwlock_r_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_rwlock_r_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	int sum = 0;
	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sum += exitstatus;
	sys_ppu_thread_join(id2, &exitstatus);
	sum += exitstatus;
	sys_ppu_thread_join(id3, &exitstatus);
	sum += exitstatus;
	sys_ppu_thread_join(id4, &exitstatus);
	sum += exitstatus;

	sys_rwlock_destroy(lock);

	printf("test_lwmutex: %d; i: %d\n", sum, i);
}

int main(void) {
	test_rwlock_w();
	test_rwlock_r();
	return 0;
}
