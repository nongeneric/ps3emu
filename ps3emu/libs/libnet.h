#pragma once

#include "sys_defs.h"

#define SYS_NET_INIT_ERROR_CHECK 0x0001
#define SYS_NET_ERROR_EBUSY 0x80010210

typedef struct sys_net_initialize_parameter {
    big_uint32_t memory;
    big_int32_t memory_size;
    big_int32_t flags;
} sys_net_initialize_parameter_t;

int32_t sys_net_initialize_network_ex(const sys_net_initialize_parameter_t* param);
int32_t sys_net_free_thread_context(uint32_t tid, int flags);
