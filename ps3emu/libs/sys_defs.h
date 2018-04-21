#pragma once

#include <stdint.h>
#include <string>
#include "../constants.h"
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
typedef big_uint64_t second_t;
typedef big_uint32_t sys_adaptive_t;
typedef big_uint64_t sys_ipc_key_t;

#define CELL_ETIMEDOUT -2147418101 /* 0x8001000B */
#define CELL_EFAULT -2147418099    /* 0x8001000D */
#define CELL_EINVAL -2147418110    /* 0x80010002 */
#define CELL_EAGAIN -2147418111    /* 0x80010001 */
#define CELL_ESRCH -2147418107     /* 0x80010005 */
#define CELL_EPERM -2147418103     /* 0x80010009 */
#define CELL_EDEADLK -2147418104   /* 0x80010008 */
#define CELL_EBUSY -2147418102     /* 0x8001000A */
#define CELL_EEXIST -2147418092    /* 0x80010014 */

struct cstring_ptr_t {
    std::string str;
};
