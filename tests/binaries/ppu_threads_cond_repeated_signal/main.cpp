#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_lwmutex_t mutex;
sys_lwcond_t cv;

int stop = 0;
int work = 0;

#define CHECK(e) { \
	uint32_t ret = e; \
	if (ret) { \
		printf(#e " failed with %x\n", ret); \
		exit(1); \
	} \
}

#define THREAD_COUNT 30

sys_ppu_thread_t threads[THREAD_COUNT];
sys_lwmutex_t ready_mutex;
int ready[THREAD_COUNT];

void thread_entry(uint64_t arg) {
	CHECK(sys_lwmutex_lock(&mutex, 0));

	sys_lwmutex_lock(&ready_mutex, 0);
	if (ready[arg]) {
		printf("why is ready[%i] == 1 ?\n", (int)arg);
		exit(1);
	}
	ready[arg] = 1;
	sys_lwmutex_unlock(&ready_mutex);

	CHECK(sys_lwcond_wait(&cv, 0));
	CHECK(sys_lwmutex_unlock(&mutex));
	sys_ppu_thread_exit(0);
}

void test_signal() {
	for (int i = 0; i < THREAD_COUNT; ++i) {
		ready[i] = 0;
	}

	for (int i = 0; i < THREAD_COUNT; ++i) {
		sys_ppu_thread_create(&threads[i], thread_entry, i, 1000, 0x1000,
			SYS_PPU_THREAD_CREATE_JOINABLE, "th");
	}

	for (;;) {
		int s = 1;
		for (int i = 0; i < THREAD_COUNT; ++i) {
			sys_lwmutex_lock(&ready_mutex, 0);
			s &= ready[i];
			sys_lwmutex_unlock(&ready_mutex);
		}
		if (s)
			break;
	}

	CHECK(sys_lwmutex_lock(&mutex, 0));
	for (int i = 0; i < THREAD_COUNT; ++i) {
		CHECK(sys_lwcond_signal(&cv));
	}
	CHECK(sys_lwmutex_unlock(&mutex));

	for (int i = 0; i < THREAD_COUNT; ++i) {
		uint64_t exitstatus;
		CHECK(sys_ppu_thread_join(threads[i], &exitstatus));
	}

	//printf("done signal\n");
}

void test_signal_all() {
	for (int i = 0; i < THREAD_COUNT; ++i) {
		ready[i] = 0;
	}

	for (int i = 0; i < THREAD_COUNT; ++i) {
		CHECK(sys_ppu_thread_create(&threads[i], thread_entry, i, 1000, 0x1000,
			SYS_PPU_THREAD_CREATE_JOINABLE, "th"));
	}

	for (;;) {
		int s = 1;
		for (int i = 0; i < THREAD_COUNT; ++i) {
			sys_lwmutex_lock(&ready_mutex, 0);
			s &= ready[i];
			sys_lwmutex_unlock(&ready_mutex);
		}
		if (s)
			break;
	}

	CHECK(sys_lwmutex_lock(&mutex, 0));
	CHECK(sys_lwcond_signal_all(&cv));
	CHECK(sys_lwmutex_unlock(&mutex));

	for (int i = 0; i < THREAD_COUNT; ++i) {
		uint64_t exitstatus;
		CHECK(sys_ppu_thread_join(threads[i], &exitstatus));
	}

	//printf("done signal all\n");
}

int main(void) {
	sys_lwmutex_attribute_t attr;
	sys_lwmutex_attribute_initialize(attr);
	CHECK(sys_lwmutex_create(&ready_mutex, &attr));

	for (int i = 0; i < 1000; ++i) {
		sys_lwmutex_attribute_t attr;
		sys_lwmutex_attribute_initialize(attr);
		CHECK(sys_lwmutex_create(&mutex, &attr));

		sys_lwcond_attribute_t cv_attr;
		sys_lwcond_attribute_initialize(cv_attr);
		CHECK(sys_lwcond_create(&cv, &mutex, &cv_attr));

		if (i % 2) {
			test_signal();
		} else {
			test_signal_all();
		}

		CHECK(sys_lwcond_destroy(&cv));
		CHECK(sys_lwmutex_destroy(&mutex));
	}

	printf("all done\n");

	return 0;
}
