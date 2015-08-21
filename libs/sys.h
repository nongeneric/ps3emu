#pragma once

#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

#define CELL_OK 0
#define CELL_STATUS_IS_FAILURE(status) ((status) & 0x80000000)
#define CELL_STATUS_IS_SUCCESS(status) (!((status) & 0x80000000))

typedef big_uint32_t _sys_sleep_queue_t;
typedef big_uint32_t sys_protocol_t;
typedef big_uint32_t sys_recursive_t;
typedef big_uint64_t usecond_t;

#define SYS_SYNC_NAME_LENGTH        7
#define SYS_SYNC_NAME_SIZE          (SYS_SYNC_NAME_LENGTH + 1)

typedef struct {
    volatile big_uint32_t owner;
    volatile big_uint32_t waiter;
} sys_lwmutex_lock_info_t;

typedef union {
    sys_lwmutex_lock_info_t info;
    volatile  big_uint64_t all_info;
} sys_lwmutex_variable_t;

typedef struct sys_lwmutex {
    sys_lwmutex_variable_t lock_var;
    big_uint32_t attribute;
    big_uint32_t recursive_count;
    _sys_sleep_queue_t sleep_queue;
    big_uint32_t pad;
} sys_lwmutex_t;

typedef struct lwmutex_attr {
    sys_protocol_t attr_protocol;   /**< Policy for waiting threads */
    sys_recursive_t attr_recursive; /**< Whether recursive locks are effective */
    char name[SYS_SYNC_NAME_SIZE]; /**< lwmutex name for debugging */
} sys_lwmutex_attribute_t;

extern int sys_lwmutex_create(sys_lwmutex_t * mutex_id,
                              sys_lwmutex_attribute_t * attr);
extern int sys_lwmutex_destroy(sys_lwmutex_t * lwmutex_id);
extern int sys_lwmutex_lock(sys_lwmutex_t * lwmutex_id, usecond_t timeout);
extern int sys_lwmutex_trylock(sys_lwmutex_t * lwmutex_id);
extern int sys_lwmutex_unlock(sys_lwmutex_t * lwmutex_id);

void sys_initialize_tls(uint64_t undef, uint32_t unk1, uint32_t unk2);

typedef struct sys_memory_info {
    big_uint64_t total_user_memory;
    big_uint64_t available_user_memory;
} sys_memory_info_t;

int sys_memory_get_user_memory_size(sys_memory_info_t * mem_info);

typedef big_int64_t system_time_t;

extern system_time_t sys_time_get_system_time();
extern int _sys_process_atexitspawn();
extern int _sys_process_at_Exitspawn();

typedef big_uint64_t sys_ppu_thread_t;

extern int sys_ppu_thread_get_id(sys_ppu_thread_t * thread_id);

extern int sys_tty_write(unsigned int ch, const void *buf,
                         unsigned int len, unsigned int *pwritelen);

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags);

int sys_prx_exitspawn_with_level(uint64_t level);
