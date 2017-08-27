#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sys/event.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_event_queue_t queue;

#define MAKE_SYS_PORT_NAME(port) \
   ((((uint64_t)sys_process_getpid()) << 32) | ((uint64_t)(uint32_t)port))

void test_cond_entry(uint64_t arg) {
	sys_event_port_t port;
	sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, 0x1317);
	sys_event_port_connect_local(port, queue);
	sys_event_port_send(port, 1, 2, 3);
	sys_ppu_thread_exit(0);
}

sys_event_port_t port2;

void test_cond_entry2(uint64_t arg) {
	sys_event_port_create(&port2, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
	sys_event_port_connect_local(port2, queue);
	sys_event_port_send(port2, 1, 2, 3);
	sys_ppu_thread_exit(0);
}

void test() {
	uint64_t exitstatus;
	int ret;
	sys_event_t event;
	
	sys_event_queue_attribute_t qattr;
	sys_event_queue_attribute_initialize(qattr);
	sys_event_queue_create(&queue, &qattr, SYS_EVENT_QUEUE_LOCAL, 32);

	sys_ppu_thread_t id1, id2;
	sys_ppu_thread_create(&id1, test_cond_entry, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	ret = sys_event_queue_receive(queue, &event, 0);
	sys_ppu_thread_join(id1, &exitstatus);
	printf("done %d: %llx %llx %llx %llx\n", ret, event.source, event.data1, event.data2, event.data3);

	sys_ppu_thread_create(&id2, test_cond_entry2, 0, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	ret = sys_event_queue_receive(queue, &event, 0);
	sys_ppu_thread_join(id2, &exitstatus);
	printf("done %d: expected=%d %llx %llx %llx\n", ret, event.source==MAKE_SYS_PORT_NAME(port2), event.data1, event.data2, event.data3);
}

int main(void) {
	test();

	return 0;
}
