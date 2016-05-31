#pragma once

#include "../sys_defs.h"
#include "../../ppu/PPUThread.h"

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
