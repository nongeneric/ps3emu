#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sys/event.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_event_queue_t queue;

void test_cond_entry(uint64_t arg) {
	sys_event_port_t port;
	sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
	sys_event_port_connect_local(port, queue);
	sys_event_port_send(port, 1, 2, 3);
	sys_ppu_thread_exit(0);
}

void test() {
	sys_event_queue_attribute_t qattr;
	sys_event_queue_attribute_initialize(qattr);
	sys_event_queue_create(&queue, &qattr, SYS_EVENT_QUEUE_LOCAL, 1);

	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");

	sys_timer_usleep(1000000);
	sys_event_queue_drain(queue);

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	
	printf("done\n");
}

int main(void) {
	test();

	return 0;
}
