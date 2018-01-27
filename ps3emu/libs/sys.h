#pragma once

#include "sys_defs.h"
#include "ps3emu/constants.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/enum.h"
#include <array>

class Process;
class PPUThread;

typedef struct sys_memory_info {
    big_uint32_t total_user_memory;
    big_uint32_t available_user_memory;
} sys_memory_info_t;


typedef struct sys_page_attr {
    big_uint64_t attribute;
    big_uint64_t access_right;
    big_uint32_t page_size;
    big_uint32_t pad;
} sys_page_attr_t;

#define SYS_MEMORY_PROT_READ_ONLY        0x0000000000080000ULL
#define SYS_MEMORY_PROT_READ_WRITE       0x0000000000040000ULL
#define SYS_MEMORY_ACCESS_RIGHT_PPU_THR  0x0000000000000008ULL
#define SYS_MEMORY_ACCESS_RIGHT_HANDLER  0x0000000000000004ULL
#define SYS_MEMORY_ACCESS_RIGHT_SPU_THR  0x0000000000000002ULL
#define SYS_MEMORY_ACCESS_RIGHT_RAW_SPU  0x0000000000000001ULL
#define SYS_MEMORY_ACCESS_RIGHT_ANY      (SYS_MEMORY_ACCESS_RIGHT_PPU_THR | \
                                          SYS_MEMORY_ACCESS_RIGHT_HANDLER | \
                                          SYS_MEMORY_ACCESS_RIGHT_SPU_THR | \
                                          SYS_MEMORY_ACCESS_RIGHT_RAW_SPU)
#define SYS_MEMORY_ACCESS_RIGHT_NONE     0x00000000000000f0ULL
#define SYS_MEMORY_ACCESS_RIGHT_MASK 0x00000000000000ffULL
#define CELL_EAGAIN -2147418111    /* 0x80010001 */
#define CELL_EINVAL -2147418110    /* 0x80010002 */
#define CELL_ENOSYS -2147418109    /* 0x80010003 */
#define CELL_ENOMEM -2147418108    /* 0x80010004 */
#define CELL_ESRCH -2147418107     /* 0x80010005 */
#define CELL_ENOENT -2147418106    /* 0x80010006 */
#define CELL_ENOEXEC -2147418105   /* 0x80010007 */
#define CELL_EDEADLK -2147418104   /* 0x80010008 */
#define CELL_EPERM -2147418103     /* 0x80010009 */
#define CELL_EBUSY -2147418102     /* 0x8001000A */
#define CELL_ETIMEDOUT -2147418101 /* 0x8001000B */
#define CELL_EABORT -2147418100    /* 0x8001000C */
#define CELL_EFAULT -2147418099    /* 0x8001000D */

void init_sys_lib();

int sys_memory_get_user_memory_size(sys_memory_info_t * mem_info);
int sys_memory_get_page_attribute(uint32_t addr, sys_page_attr_t* attr);
extern int _sys_process_atexitspawn();
extern int _sys_process_at_Exitspawn();

typedef big_uint64_t sys_ppu_thread_t;

int sys_tty_write(uint32_t ch,
                  uint32_t buf_va,
                  uint32_t buf_len,
                  uint32_t* pwritelen);

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags);

typedef big_uint32_t sys_prx_id_t;

struct sys_prx_load_module_option_t {
    big_uint64_t size; // TODO: !
};

int sys_prx_exitspawn_with_level(uint64_t level);
sys_prx_id_t sys_prx_load_module(cstring_ptr_t path, uint64_t flags, uint64_t opt, Process* proc);
sys_prx_id_t sys_prx_load_module_on_memcontainer(

    cstring_ptr_t path,

    uint32_t mem_container,

    uint64_t flags,

    uint32_t pOpt);

ENUM(prx_module_mode,
    (user_lookup, 1),
    (user_confirm, 2),
    (system_lookup, 4),
    (system_confirm, 8)
)

struct sys_prx_start_module_t {
    big_uint64_t struct_size;
    big_uint64_t mode;
    big_uint64_t name_or_fdescrva;
    big_uint64_t unk2;
    big_uint64_t neg1;
};
static_assert(sizeof(sys_prx_start_module_t) == 0x28, "");

int32_t sys_prx_start_module(sys_prx_id_t id,
                             uint64_t flags,
                             sys_prx_start_module_t* opt,
                             PPUThread* thread);
int32_t sys_prx_stop_module(sys_prx_id_t id,
                            uint64_t flags,
                            sys_prx_start_module_t* opt,
                            PPUThread* thread);
int32_t sys_prx_unload_module(sys_prx_id_t id, uint64_t flags, uint64_t pOpt);

typedef big_uint32_t sys_addr_t;

#define SYS_MEMORY_GRANULARITY_1M        0x0000000000000400ULL
#define SYS_MEMORY_GRANULARITY_64K       0x0000000000000200ULL
#define SYS_MEMORY_GRANULARITY_MASK      0x0000000000000f00ULL

int sys_memory_allocate(uint32_t size, uint64_t flags, sys_addr_t * alloc_addr, PPUThread* thread);
int sys_memory_free(ps3_uintptr_t start_addr, PPUThread* thread);
int sys_timer_usleep(usecond_t sleep_time);
int sys_timer_sleep(second_t sleep_time);

uint32_t sys_time_get_timebase_frequency(PPUThread* thread);
uint32_t sys_time_get_current_time(big_int64_t* sec, big_int64_t* nsec);
uint32_t sys_time_get_timezone(big_uint32_t* timezone, big_uint32_t* summertime);

typedef struct {
    big_uint32_t pst_addr;
    big_uint32_t pst_size;
} sys_ppu_thread_stack_t;

int32_t sys_ppu_thread_get_stack_information(sys_ppu_thread_stack_t* info, PPUThread* thread);

uint32_t cellSysmoduleInitialize();
uint32_t cellSysmoduleLoadModule(uint16_t id);
uint32_t cellSysmoduleUnloadModule(uint16_t id);
uint32_t cellSysmoduleIsLoaded(uint16_t id);

// semaphore

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
int32_t sys_semaphore_get_value(sys_semaphore_t sem, sys_semaphore_value_t* val);

// ppu thread

typedef big_uint64_t sys_ppu_thread_t;

struct ppu_thread_create_t {
    big_uint32_t entry_fdescr_va;
    big_uint32_t tls_va;
};

int32_t sys_ppu_thread_create(sys_ppu_thread_t* thread_id,
                              const ppu_thread_create_t* info,
                              uint64_t arg,
                              uint64_t unk,
                              uint32_t prio,
                              uint32_t stacksize,
                              uint64_t flags,
                              cstring_ptr_t threadname);
int32_t sys_ppu_thread_start(sys_ppu_thread_t id);
int32_t sys_ppu_thread_join(sys_ppu_thread_t thread_id, big_uint64_t* exit_code, Process* proc);
int32_t sys_ppu_thread_detach(sys_ppu_thread_t thread_id);
int32_t sys_ppu_thread_exit(uint64_t code, PPUThread* thread);
int32_t sys_ppu_thread_set_priority(sys_ppu_thread_t thread_id, int32_t prio, Process* proc);
emu_void_t sys_ppu_thread_yield(PPUThread* thread);
int32_t sys_ppu_thread_get_priority(sys_ppu_thread_t thread_id, big_int32_t* prio, Process* proc);

emu_void_t sys_process_exit(int32_t status);

int32_t sys_process_is_spu_lock_line_reservation_address(ps3_uintptr_t addr, uint64_t flags);
uint32_t sys_process_getpid();
int32_t sys_process_get_sdk_version(uint32_t pid, big_uint32_t* version);
sys_prx_id_t sys_prx_get_module_id_by_name(cstring_ptr_t name,
                                           uint64_t flags,
                                           uint64_t pOpt,
                                           PPUThread* thread);
int32_t _sys_printf(cstring_ptr_t format, PPUThread* thread);
int32_t _sys_process_get_paramsfo(std::array<char, 0x40>* sfo);

struct process_info_t {
    big_uint64_t size;
    big_uint64_t unk1;
    big_uint64_t unk2;
    big_uint64_t unk_incr;
    big_uint64_t unk_incr_2;
    big_uint64_t modules;
    big_uint64_t cond_vars;
    big_uint64_t mutexes;
    big_uint64_t event_queues;
    big_uint64_t unk3;
};
static_assert(sizeof(process_info_t) == 0x50, "");

int32_t sys_get_process_info(process_info_t* info);
int32_t sys_prx_register_module(cstring_ptr_t name, uint64_t opts);
int32_t sys_prx_register_library(uint32_t va);
int32_t sys_ss_access_control_engine(uint64_t unk1, uint64_t unk2, uint64_t unk3);
int32_t sys_prx_load_module_list(int32_t n,
                                 ps3_uintptr_t path_list,
                                 uint64_t flags,
                                 uint64_t pOpt,
                                 ps3_uintptr_t idlist);
int32_t sys_mmapper_allocate_shared_memory(uint32_t id,
                                           uint32_t size,
                                           uint32_t alignment,
                                           big_uint32_t* mem);
int32_t sys_mmapper_allocate_address(uint32_t size,
                                     uint64_t flags,
                                     uint32_t alignment,
                                     big_uint32_t* mem);
int32_t sys_mmapper_search_and_map(uint32_t start_addr,
                                   uint32_t mem_id,
                                   uint64_t flags,
                                   big_uint32_t* alloc_addr);

struct sys_prx_get_module_list_t {
    big_uint64_t size;
    big_uint32_t max_levels_size;
    big_uint32_t max_ids_size;
    big_uint32_t out_count;
    big_uint32_t ids_va;
    big_uint32_t levels_va;
    big_uint32_t unk1;
};
static_assert(sizeof(sys_prx_get_module_list_t) == 0x20, "");

int32_t sys_prx_get_module_list(uint32_t flags, sys_prx_get_module_list_t* info);
emu_void_t _sys_memcpy(uint64_t dest, uint64_t src, uint64_t size, MainMemory* mm);
emu_void_t _sys_memset(uint64_t dest, uint64_t value, uint64_t size, MainMemory* mm);

uint32_t emuEmptyModuleStart();
