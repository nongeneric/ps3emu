#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_mutex_t mutex;
sys_cond_t cv;

void test_cond_entry(uint64_t arg) {
	sys_mutex_lock(mutex, 0);
	int res = sys_cond_wait(cv, 3000000);
	sys_mutex_unlock(mutex);
	sys_ppu_thread_exit(res);
}

void test_all() {
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_create(&mutex, &attr);

	sys_cond_attribute_t cv_attr;
	sys_cond_attribute_initialize(cv_attr);
	sys_cond_create(&cv, mutex, &cv_attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	sys_timer_usleep(1000000);

	sys_cond_signal_all(cv);

	int ok = 0, timeout = 0;
	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id2, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id3, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id4, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;

	sys_cond_destroy(cv);
	sys_mutex_destroy(mutex);

	printf("test_all: ok=%d timeout=%d\n", ok, timeout);
}

void test_one() {
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_create(&mutex, &attr);

	sys_cond_attribute_t cv_attr;
	sys_cond_attribute_initialize(cv_attr);
	sys_cond_create(&cv, mutex, &cv_attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	sys_timer_usleep(1000000);

	sys_cond_signal(cv);

	int ok = 0, timeout = 0;
	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id2, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id3, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;
	sys_ppu_thread_join(id4, &exitstatus);
	ok += exitstatus == CELL_OK;
	timeout += exitstatus == ETIMEDOUT;

	sys_cond_destroy(cv);
	sys_mutex_destroy(mutex);

	printf("test_one: ok=%d timeout=%d\n", ok, timeout);
}

void test_to() {
	sys_mutex_attribute_t attr;
	sys_mutex_attribute_initialize(attr);
	sys_mutex_create(&mutex, &attr);

	sys_cond_attribute_t cv_attr;
	sys_cond_attribute_initialize(cv_attr);
	sys_cond_create(&cv, mutex, &cv_attr);
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	sys_timer_usleep(1000000);

	sys_cond_signal_to(cv, id3);

	uint64_t e1, e2, e3, e4;
	sys_ppu_thread_join(id1, &e1);
	sys_ppu_thread_join(id2, &e2);
	sys_ppu_thread_join(id3, &e3);
	sys_ppu_thread_join(id4, &e4);

	sys_cond_destroy(cv);
	sys_mutex_destroy(mutex);

	printf("test_to_3(ok): e1=%d e2=%d e3=%d e4=%d\n", 
		e1 == CELL_OK, e2 == CELL_OK, e3 == CELL_OK, e4 == CELL_OK);
}

int main(void) {
	test_all();
	test_one();
	test_to();

	return 0;
}
