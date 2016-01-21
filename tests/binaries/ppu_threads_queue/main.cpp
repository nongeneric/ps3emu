#include <stdlib.h>
#include <stdio.h>
#include <sys/process.h>
#include <sys/ppu_thread.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sys/event.h>

SYS_PROCESS_PARAM(1001, 0x10000)

sys_event_queue_t queue;
int portname = 10;

void entry_1(uint64_t arg) {
	int* i = (int*)arg;
	int j = 0;
	for (;;) {
		if (j++ % 3) {
			sys_event_t ev;
			sys_event_queue_receive(queue, &ev, 0);
			if (ev.data3 == 7)
				sys_ppu_thread_exit(0);
			__sync_fetch_and_add(i, ev.source);
			__sync_fetch_and_add(i, ev.data1);
			__sync_fetch_and_add(i, ev.data2);
			__sync_fetch_and_add(i, ev.data3);
		} else {
			sys_event_t evs[3];
			int num;
			sys_event_queue_tryreceive(queue, evs, 3, &num);
			for (int n = 0; n < num; ++n) {
				if (evs[n].data3 == 7)
					sys_ppu_thread_exit(0);
				__sync_fetch_and_add(i, evs[n].source);
				__sync_fetch_and_add(i, evs[n].data1);
				__sync_fetch_and_add(i, evs[n].data2);
				__sync_fetch_and_add(i, evs[n].data3);
			}
		}
	}
}

void test_correctness(bool priority) {
	int i = 0;

	sys_event_queue_attr attr;
	sys_event_queue_attribute_initialize(attr);
	attr.attr_protocol = priority ? SYS_SYNC_PRIORITY : SYS_SYNC_FIFO;
	sys_event_queue_create(&queue, &attr, SYS_EVENT_QUEUE_LOCAL, 110);

	sys_event_port_t port;
	sys_event_port_create(&port, SYS_EVENT_PORT_LOCAL, portname);
	sys_event_port_connect_local(port, queue);
	
	// this relies on the events being processed in fifo order
	// (unrelated to the priority/fifo distinction of the serving of waiting threads)
	for (int i = 0; i < 100; ++i) {
		sys_event_port_send(port, 0, 1, 1);
	}

	sys_ppu_thread_t id1, id2, id3, id4;
	sys_ppu_thread_create(&id1, entry_1, (uint64_t)&i, 1000, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th1");
	sys_ppu_thread_create(&id2, entry_1, (uint64_t)&i, 1100, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th2");
	sys_ppu_thread_create(&id3, entry_1, (uint64_t)&i, 1200, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th3");
	sys_ppu_thread_create(&id4, entry_1, (uint64_t)&i, 1300, 0x1000,
		SYS_PPU_THREAD_CREATE_JOINABLE, "th4");

	for (int i = 0; i < 40000; ++i) {
		while (sys_event_port_send(port, 1, 1, 0) == EBUSY) ;
	}

	for (int i = 0; i < 3 * 4; ++i) {
		while (sys_event_port_send(port, 0, 0, 7) == EBUSY) ;
	}

	uint64_t exitstatus;
	sys_ppu_thread_join(id1, &exitstatus);
	sys_ppu_thread_join(id2, &exitstatus);
	sys_ppu_thread_join(id3, &exitstatus);
	sys_ppu_thread_join(id4, &exitstatus);

	sys_event_queue_destroy(queue, 0);

	printf("test_correctness(%d): %d; i: %d\n", priority, exitstatus, i);
}

int main(void) {
	test_correctness(true);
	test_correctness(false);
	return 0;
}
