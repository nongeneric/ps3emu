#pragma once

#include "../sys_defs.h"
#include "ps3emu/ppu/PPUThread.h"

#define SYS_SYNC_FIFO 0x00001
#define SYS_SYNC_PRIORITY 0x00002
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL 0x01
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_SPU_THREAD_EVENT_USER 0x1

typedef struct sys_event_queue_attr {
    sys_protocol_t attr_protocol;
    big_uint32_t type;
    char name[SYS_SYNC_NAME_SIZE];
} sys_event_queue_attribute_t;

struct sys_event_t {
    big_uint64_t source;
    big_uint64_t data1;
    big_uint64_t data2;
    big_uint64_t data3;
};

typedef big_uint32_t sys_event_queue_t;
typedef big_uint32_t sys_event_port_t;
typedef big_uint32_t sys_event_type_t;

static_assert(sizeof(sys_event_t) == 32, "");

class Process;

int32_t sys_event_queue_create(sys_event_queue_t* equeue_id,
                               sys_event_queue_attribute_t* attr,
                               sys_ipc_key_t event_queue_key,
                               uint32_t size);

int32_t sys_event_queue_destroy(sys_event_queue_t equeue_id, int32_t mode);

int32_t sys_event_queue_receive(sys_event_queue_t equeue_id,
                                uint32_t unused,
                                usecond_t timeout,
                                PPUThread* th);

int32_t sys_event_queue_tryreceive(sys_event_queue_t equeue_id,
                                   uint32_t event_array,
                                   int32_t size,
                                   big_uint32_t *number,
                                   PPUThread* th);

int32_t sys_event_port_create(sys_event_port_t* eport_id,
                              int32_t port_type,
                              uint64_t name);

int32_t sys_event_port_connect_local(sys_event_port_t event_port_id,
                                     sys_event_queue_t event_queue_id);

int32_t sys_event_port_disconnect(sys_event_port_t event_port_id);

int32_t sys_event_port_send(sys_event_port_t eport_id,
                            uint64_t data1,
                            uint64_t data2,
                            uint64_t data3);

int32_t sys_event_queue_drain(sys_event_queue_t equeue_id);

int32_t sys_event_port_destroy(sys_event_port_t eport_id);

int32_t sys_spu_thread_connect_event(uint32_t thread_id,
                                     sys_event_queue_t eq,
                                     sys_event_type_t et,
                                     uint8_t spup);

int32_t sys_spu_thread_disconnect_event(uint32_t thread_id,
                                        sys_event_type_t et,
                                        uint8_t spup);

int32_t sys_spu_thread_unbind_queue(uint32_t thread_id, uint32_t spuq_num);

int32_t sys_spu_thread_bind_queue(uint32_t thread_id,
                                  sys_event_queue_t spuq,
                                  uint32_t spuq_num);

int32_t sys_spu_thread_group_connect_event_all_threads(uint32_t group_id,
                                                       sys_event_queue_t eq,
                                                       uint64_t req,
                                                       uint8_t* spup);

sys_event_queue_t getQueueByKey(sys_ipc_key_t key);
void disableLogging(sys_event_queue_t queue);
