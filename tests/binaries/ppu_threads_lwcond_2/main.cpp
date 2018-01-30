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

void consumer(uint64_t arg) {
	int report = 100;
	int count = 0;

	CHECK(sys_lwmutex_lock(&mutex, 0));
	printf("consumer started\n");
	for (;;) {
		CHECK(sys_lwcond_wait(&cv, 0));
		count += work;
		if (stop) {
			sys_lwmutex_unlock(&mutex);
			sys_ppu_thread_exit(count);
		}
		if (count > report) {
			//printf("processed more than %d\n", report);
			report += 100;
		}
		work = 0;
	}
}

void producer_one(uint64_t arg) {
	for (int i = 0; i < 50000; ++i) {
		CHECK(sys_lwmutex_lock(&mutex, 0));
		work++;
		CHECK(sys_lwmutex_unlock(&mutex));
		CHECK(sys_lwcond_signal(&cv));
	}
	sys_ppu_thread_exit(0);
}

void test_3producers_1consumer() {
	stop = 0;
	work = 0;
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, consumer, 1, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, producer_one, 2, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, producer_one, 3, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, producer_one, 4, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");
	
	uint64_t exitstatus;
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	printf("producers done\n");

	sys_timer_usleep(200);

	sys_lwmutex_lock(&mutex, 0);
	printf("setting stop to 1\n");
	stop = 1;
	sys_lwmutex_unlock(&mutex);
	sys_lwcond_signal(&cv);

	uint64_t count;
	sys_ppu_thread_join(id1, &count);

	printf("count: %lld\n", count);
}

void producer_all(uint64_t arg) {
	for (int i = 0; i < 50000; ++i) {
		CHECK(sys_lwmutex_lock(&mutex, 0));
		work++;
		CHECK(sys_lwmutex_unlock(&mutex));
		CHECK(sys_lwcond_signal_all(&cv));
	}
	sys_ppu_thread_exit(0);
}

void test_2producers_2consumers() {
	stop = 0;
	work = 0;
	
	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, consumer, 1, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, consumer, 2, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, producer_all, 3, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, producer_all, 4, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");
	
	uint64_t exitstatus;
	CHECK(sys_ppu_thread_join(id3, &exitstatus));
	CHECK(sys_ppu_thread_join(id4, &exitstatus));

	printf("producers done\n");

	sys_timer_usleep(200);

	CHECK(sys_lwmutex_lock(&mutex, 0));
	printf("setting stop to 1\n");
	stop = 1;
	CHECK(sys_lwmutex_unlock(&mutex));
	printf("signalling\n");
	CHECK(sys_lwcond_signal_all(&cv));

	uint64_t count1, count2;
	CHECK(sys_ppu_thread_join(id1, &count1));
	CHECK(sys_ppu_thread_join(id2, &count2));

	printf("count: %lld\n", count1 + count2);
}

int main(void) {
	sys_lwmutex_attribute_t attr;
	sys_lwmutex_attribute_initialize(attr);
	CHECK(sys_lwmutex_create(&mutex, &attr));

	sys_lwcond_attribute_t cv_attr;
	sys_lwcond_attribute_initialize(cv_attr);
	CHECK(sys_lwcond_create(&cv, &mutex, &cv_attr));

	test_2producers_2consumers();
	test_3producers_1consumer();

	CHECK(sys_lwcond_destroy(&cv));
	CHECK(sys_lwmutex_destroy(&mutex));

	return 0;
}
