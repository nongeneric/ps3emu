#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <assert.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_event_flag_t flag;
int p = 1;

void test_and(uint64_t mask) {
	uint64_t flag_value = 0xbadbadbadull;
	int res = sys_event_flag_wait(flag, mask, SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR, &flag_value, SYS_NO_TIMEOUT);
	assert(res == 0);
	assert((flag_value & mask) == mask);
	p *= 3 + flag_value;
	printf("p = %x, flag = %x\n", p, flag_value);
	sys_ppu_thread_exit(0);
}

void test_or(uint64_t mask) {
	uint64_t flag_value = 0xbadbadbadull;
	int res = sys_event_flag_wait(flag, mask, SYS_EVENT_FLAG_WAIT_OR | SYS_EVENT_FLAG_WAIT_CLEAR_ALL, &flag_value, SYS_NO_TIMEOUT);
	assert(res == 0);
	assert(flag_value & mask);
	p *= 7 + flag_value;
	printf("p = %x, flag = %x\n", p, flag_value);
	sys_ppu_thread_exit(0);
}

void test_event_flag() {
	sys_event_flag_attribute_t attr;
	sys_event_flag_attribute_initialize(attr);
	int res = sys_event_flag_create(&flag, &attr, 0x8000);
	assert(res == 0);
	
	sys_ppu_thread_t id1, id2, id3;
	sys_ppu_thread_create(&id1, test_and, 5, 1000, 0x1000, // 0101
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_or, 8, 1000, 0x1000,  // 1000
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, test_and, 22, 1000, 0x1000, // 10110
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");

	uint64_t exitstatus, value;

	res = sys_event_flag_set(flag, 5); // wake up th1
	assert(res == 0);
	sys_ppu_thread_join(id1, &exitstatus);

	sys_event_flag_get(flag, &value);
	printf("flag value: %x\n", value);

	res = sys_event_flag_set(flag, 28); // wake up th2
	assert(res == 0);
	sys_ppu_thread_join(id2, &exitstatus);

	sys_event_flag_get(flag, &value);
	printf("flag value: %x\n", value);

	res = sys_event_flag_set(flag, 470); // wake up th3
	assert(res == 0);
	sys_ppu_thread_join(id3, &exitstatus);

	sys_event_flag_get(flag, &value);
	printf("flag value: %x\n", value);

	printf("test_event_flag: %d; p: %d\n", exitstatus, p);

	res = sys_event_flag_clear(flag, -1);

	sys_event_flag_get(flag, &value);
	printf("flag value (clear 0): %x\n", value);

	res = sys_event_flag_clear(flag, 0xf0);
	sys_event_flag_get(flag, &value);
	printf("flag value (clear 1): %x\n", value);

	assert(res == 0);
	sys_ppu_thread_create(&id1, test_and, 5, 1000, 0x1000, // 0101
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	res = sys_event_flag_set(flag, 15);
	assert(res == 0);
	sys_ppu_thread_join(id1, &exitstatus);

	sys_event_flag_get(flag, &value);
	printf("flag value: %x\n", value);

	printf("test_event_flag: %d; p: %d\n", exitstatus, p);

	res = sys_event_flag_destroy(flag);
	assert(res == 0);
}


int main(void) {
	test_event_flag();
	return 0;
}
