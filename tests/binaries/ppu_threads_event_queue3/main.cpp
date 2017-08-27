#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sys/event.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_event_queue_t queue;

void test() {
	sys_event_queue_attribute_t qattr;
	sys_event_queue_attribute_initialize(qattr);
	sys_event_queue_create(&queue, &qattr, SYS_EVENT_QUEUE_LOCAL, 3);

	int ret;
	sys_event_port_t port;
	sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
	sys_event_port_connect_local(port, queue);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);

	ret = sys_event_queue_drain(queue);
	printf("ret=%x (drain)\n", ret);

	ret = sys_event_port_send(port, 1, 2, 3);
	printf("ret=%x\n", ret);
}

int main(void) {
	test();

	return 0;
}
