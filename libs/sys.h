#pragma once

#include "sys_defs.h"
#include "../ps3emu/constants.h"
#include "../ps3emu/ELFLoader.h"

typedef big_uint32_t _sys_sleep_queue_t;
typedef big_uint32_t sys_protocol_t;
typedef big_uint32_t sys_recursive_t;
typedef big_uint32_t sys_process_shared_t;
typedef big_uint64_t usecond_t;
typedef big_uint32_t sys_adaptive_t;

#define SYS_SYNC_NAME_LENGTH        7
#define SYS_SYNC_NAME_SIZE          (SYS_SYNC_NAME_LENGTH + 1)

class Process;
class PPUThread;

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
    sys_protocol_t attr_protocol;
    sys_recursive_t attr_recursive;
    char name[SYS_SYNC_NAME_SIZE];
} sys_lwmutex_attribute_t;

void init_sys_lib();

extern int sys_lwmutex_create(sys_lwmutex_t * mutex_id,
                              sys_lwmutex_attribute_t * attr);
extern int sys_lwmutex_destroy(sys_lwmutex_t * lwmutex_id);
extern int sys_lwmutex_lock(sys_lwmutex_t * lwmutex_id, usecond_t timeout);
extern int sys_lwmutex_trylock(sys_lwmutex_t * lwmutex_id);
extern int sys_lwmutex_unlock(sys_lwmutex_t * lwmutex_id);

typedef struct sys_memory_info {
    big_uint32_t total_user_memory;
    big_uint32_t available_user_memory;
} sys_memory_info_t;

int sys_memory_get_user_memory_size(sys_memory_info_t * mem_info);

extern cell_system_time_t sys_time_get_system_time(PPUThread* thread);
extern int _sys_process_atexitspawn();
extern int _sys_process_at_Exitspawn();

typedef big_uint64_t sys_ppu_thread_t;

extern int sys_ppu_thread_get_id(sys_ppu_thread_t * thread_id);

extern int sys_tty_write(unsigned int ch, const void *buf,
                         unsigned int len, unsigned int *pwritelen);

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags);

int sys_prx_exitspawn_with_level(uint64_t level);

typedef big_uint32_t sys_addr_t;

#define SYS_MEMORY_GRANULARITY_1M        0x0000000000000400ULL
#define SYS_MEMORY_GRANULARITY_64K       0x0000000000000200ULL
#define SYS_MEMORY_GRANULARITY_MASK      0x0000000000000f00ULL

int sys_memory_allocate(uint32_t size, uint64_t flags, sys_addr_t * alloc_addr, PPUThread* thread);
int sys_timer_usleep(usecond_t sleep_time);

typedef struct sys_event_queue_attr {
    sys_protocol_t attr_protocol;
    big_uint32_t type;
    char name[SYS_SYNC_NAME_SIZE];
} sys_event_queue_attribute_t;

typedef struct sys_event {
    big_uint64_t source;
    big_uint64_t data1;
    big_uint64_t data2;
    big_uint64_t data3;
} sys_event_t;

typedef big_uint32_t sys_event_queue_t;
typedef big_uint32_t sys_event_port_t;
typedef big_uint32_t sys_event_type_t;
typedef big_uint64_t sys_ipc_key_t;

int sys_event_queue_create(sys_event_queue_t* equeue_id,
                           sys_event_queue_attribute_t* attr,
                           sys_ipc_key_t event_queue_key,
                           uint32_t size);

int sys_event_port_create(sys_event_port_t* eport_id,
                          int port_type, uint64_t name);

int sys_event_port_connect_local(sys_event_port_t event_port_id,
                                 sys_event_queue_t event_queue_id);

uint32_t sys_time_get_timebase_frequency(PPUThread* thread);
uint32_t sys_time_get_current_time(int64_t* sec, int64_t* nsec);
uint32_t sys_time_get_timezone(uint32_t* timezone, uint32_t* summertime);

typedef struct {
    big_uint32_t pst_addr;
    big_uint32_t pst_size;
} sys_ppu_thread_stack_t;

int32_t sys_ppu_thread_get_stack_information(sys_ppu_thread_stack_t* info, PPUThread* thread);

uint32_t _sys_heap_create_heap(big_uint32_t* id, uint32_t size, uint32_t unk2, uint32_t unk3);
uint32_t cellSysmoduleInitialize();
uint32_t cellSysmoduleLoadModule(uint16_t id);
uint32_t cellSysmoduleUnloadModule(uint16_t id);
uint32_t cellSysmoduleIsLoaded(uint16_t id);

// mutex

typedef big_uint32_t sys_mutex_t;

struct sys_mutex_attribute_t {
    sys_protocol_t attr_protocol;
    sys_recursive_t attr_recursive;
    sys_process_shared_t attr_pshared;
    sys_adaptive_t attr_adaptive;
    sys_ipc_key_t key;
    big_uint32_t flags;
    uint32_t pad;
    char name[SYS_SYNC_NAME_SIZE];
};
static_assert(sizeof(sys_mutex_attribute_t) == 40, "");

int sys_mutex_create(sys_mutex_t* mutex_id, sys_mutex_attribute_t* attr);
int sys_mutex_destroy(sys_mutex_t mutex_id);
int sys_mutex_lock(sys_mutex_t mutex_id, usecond_t timeout);
int sys_mutex_trylock(sys_mutex_t mutex_id);
int sys_mutex_unlock(sys_mutex_t mutex_id);

// semaphore

static constexpr uint32_t CELL_EBUSY = 0x8001000A;
static constexpr uint32_t CELL_ETIMEDOUT = 0x8001000B;

typedef big_uint32_t sys_semaphore_t;
typedef big_uint32_t sys_semaphore_value_t;

struct sys_semaphore_attribute_t {
    sys_protocol_t attr_protocol;
    sys_process_shared_t attr_pshared;
    sys_ipc_key_t key;
    big_uint32_t flags;
    uint32_t pad;
    char name[SYS_SYNC_NAME_SIZE];
};
static_assert(sizeof(sys_semaphore_attribute_t) == 32, "");

int32_t sys_semaphore_create(sys_semaphore_t* sem,
                             sys_semaphore_attribute_t* attr,
                             sys_semaphore_value_t initial_val,
                             sys_semaphore_value_t max_val);
int32_t sys_semaphore_destroy(sys_semaphore_t sem);
int32_t sys_semaphore_wait(sys_semaphore_t sem, usecond_t timeout);
int32_t sys_semaphore_trywait(sys_semaphore_t sem);
int32_t sys_semaphore_post(sys_semaphore_t sem, sys_semaphore_value_t val);

// ppu thread

typedef big_uint64_t sys_ppu_thread_t;

int32_t sys_ppu_thread_create(sys_ppu_thread_t* thread_id,
                              ps3_uintptr_t entry,
                              uint64_t arg,
                              uint32_t prio,
                              uint32_t stacksize,
                              uint64_t flags,
                              const char *threadname,
                              Process* proc);
int32_t sys_ppu_thread_join(sys_ppu_thread_t thread_id, big_uint64_t* exit_code, Process* proc);
int32_t sys_ppu_thread_exit(uint64_t code, PPUThread* thread);

emu_void_t sys_process_exit(PPUThread* thread);
emu_void_t sys_initialize_tls(uint64_t undef, uint64_t unk1, uint64_t unk2, PPUThread* thread);
