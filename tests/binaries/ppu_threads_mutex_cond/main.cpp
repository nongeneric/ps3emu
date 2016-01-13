#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_mutex_t mutex;
sys_cond_t cv;

void test_mutex_entry(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		sys_mutex_lock(mutex, 10000000);
		int tmp = *i;
		sys_timer_usleep(10);
		*i = tmp + 1;
		sys_mutex_unlock(mutex);
	}
	sys_ppu_thread_exit(0);
}
void test_mutex_entry2(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		while (sys_mutex_trylock(mutex) != CELL_OK)
			sys_timer_usleep(10);
		(*i)++;
		sys_mutex_unlock(mutex);
	}
	sys_ppu_thread_exit(0);
}

void test_mutex() {
	int i = 0;

	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_create(&mutex, &attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_mutex_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_mutex_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_mutex_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_mutex_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	sys_mutex_destroy(mutex);

	printf("test_mutex: %d; i: %d\n", exitstatus, i);
}

void test_mutex_recursive_entry(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		sys_mutex_lock(mutex, 10000000);
		sys_mutex_lock(mutex, 10000000);
		sys_mutex_lock(mutex, 10000000);
		sys_mutex_lock(mutex, 10000000);
		(*i)++;
		sys_mutex_unlock(mutex);
		sys_mutex_unlock(mutex);
		sys_mutex_unlock(mutex);
		sys_mutex_unlock(mutex);
	}
	sys_ppu_thread_exit(0);
}

void test_mutex_recursive_entry2(uint64_t arg) {
	int* i = (int*)arg;
	for (int n = 0; n < 1000; ++n) {
		while (sys_mutex_trylock(mutex) != CELL_OK)
			sys_timer_usleep(10);
		while (sys_mutex_trylock(mutex) != CELL_OK)
			sys_timer_usleep(10);
		while (sys_mutex_trylock(mutex) != CELL_OK)
			sys_timer_usleep(10);
		(*i)++;
		sys_mutex_unlock(mutex);
		sys_mutex_unlock(mutex);
		sys_mutex_unlock(mutex);
	}
	sys_ppu_thread_exit(0);
}

void test_mutex_recursive() {
	int i = 0;

	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	attr.attr_recursive = SYS_SYNC_RECURSIVE;
	sys_mutex_create(&mutex, &attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_mutex_recursive_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_mutex_recursive_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_mutex_recursive_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_mutex_recursive_entry2, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	sys_mutex_destroy(mutex);

	printf("test_mutex_recursive: %d; i: %d\n", exitstatus, i);
}

bool stop = false;

void test_cond_entry(uint64_t arg) {
	int sum = 0;
	int* i = (int*)arg;
	sys_mutex_lock(mutex, 0);
	for (;;) {
		while (*i == 0 && !stop)
			sys_cond_wait(cv, 0);
		if (stop && *i == 0) {
			sys_mutex_unlock(mutex);
			sys_ppu_thread_exit(sum);
		}
		sum++;
		(*i)--;
	}
}

void test_cond() {
	int i = 0;
	
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_create(&mutex, &attr);

	sys_cond_attribute_t cv_attr;
	sys_cond_attribute_initialize(cv_attr);
	sys_cond_create(&cv, mutex, &cv_attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_cond_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_cond_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_cond_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_cond_entry, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	sys_mutex_lock(mutex, 0);
	i += 5;
	sys_cond_signal(cv);
	sys_mutex_unlock(mutex);

	sys_mutex_lock(mutex, 0);
	i += 10;
	sys_cond_signal_all(cv);
	sys_mutex_unlock(mutex);

	sys_mutex_lock(mutex, 0);
	i += 5000;
	stop = true;
	sys_cond_signal_all(cv);
	sys_mutex_unlock(mutex);

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

	sys_cond_destroy(cv);
	sys_mutex_destroy(mutex);

	printf("test_cond: %d; i: %d\n", sum, i);
}

int main(void) {
	test_mutex();
	test_mutex_recursive();
	test_cond();

	return 0;
}
