#pragma once

#include "../sys_defs.h"

typedef struct {
    sys_protocol_t attr_protocol;
    sys_process_shared_t attr_pshared;
    sys_ipc_key_t key;
    int flags;
    int type;
    char name[SYS_SYNC_NAME_SIZE];
} sys_event_flag_attribute_t;

#define SYS_EVENT_FLAG_WAIT_AND            0x000001
#define SYS_EVENT_FLAG_WAIT_OR             0x000002
#define SYS_EVENT_FLAG_WAIT_CLEAR          0x000010
#define SYS_EVENT_FLAG_WAIT_CLEAR_ALL      0x000020

int32_t sys_event_flag_create(big_uint32_t* id,
                              const sys_event_flag_attribute_t* attr,
                              uint64_t init);
int32_t sys_event_flag_destroy(uint32_t id);
int32_t sys_event_flag_wait(uint32_t id,
                            uint64_t bitptn,
                            uint32_t mode,
                            big_uint64_t* result,
                            usecond_t timeout);
int32_t sys_event_flag_set(uint32_t id, uint64_t bitptn);
int32_t sys_event_flag_get(uint32_t id, big_uint64_t* value);
int32_t sys_event_flag_clear(uint32_t id, uint64_t bitptn);
