#pragma once

#include <stdint.h>
#include <string>
#include "../ps3emu/constants.h"
#include <boost/endian/arithmetic.hpp>
using namespace boost::endian;

#define CELL_OK 0
#define CELL_STATUS_IS_FAILURE(status) ((status) & 0x80000000)
#define CELL_STATUS_IS_SUCCESS(status) (!((status) & 0x80000000))

#define SYS_SYNC_NAME_LENGTH        7
#define SYS_SYNC_NAME_SIZE          (SYS_SYNC_NAME_LENGTH + 1)
#define   SYS_SYNC_RECURSIVE                0x00010
#define   SYS_SYNC_NOT_RECURSIVE            0x00020

typedef big_int64_t cell_time_t;
typedef big_int64_t cell_system_time_t;
typedef big_uint32_t _sys_sleep_queue_t;
typedef big_uint32_t _sys_lwcond_queue_t;
typedef big_uint32_t sys_protocol_t;
typedef big_uint32_t sys_recursive_t;
typedef big_uint32_t sys_process_shared_t;
typedef big_uint64_t usecond_t;
typedef big_uint32_t sys_adaptive_t;
typedef big_uint64_t sys_ipc_key_t;

static constexpr uint32_t CELL_EBUSY = 0x8001000A;
static constexpr uint32_t CELL_ETIMEDOUT = 0x8001000B;

struct cstring_ptr_t {
    std::string str;
};