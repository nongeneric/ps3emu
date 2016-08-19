#pragma once

#include "sys_defs.h"
#include "../constants.h"
#include "../ELFLoader.h"
#include <array>

class Process;
class PPUThread;

typedef struct sys_memory_info {
    big_uint32_t total_user_memory;
    big_uint32_t available_user_memory;
} sys_memory_info_t;

void init_sys_lib();

int sys_memory_get_user_memory_size(sys_memory_info_t * mem_info);

extern cell_system_time_t sys_time_get_system_time(PPUThread* thread);
extern int _sys_process_atexitspawn();
extern int _sys_process_at_Exitspawn();

typedef big_uint64_t sys_ppu_thread_t;

extern int sys_tty_write(unsigned int ch, const void *buf,
                         unsigned int len, unsigned int *pwritelen);

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags);

typedef big_uint32_t sys_prx_id_t;

int sys_prx_exitspawn_with_level(uint64_t level);
sys_prx_id_t sys_prx_load_module(cstring_ptr_t path, uint64_t flags, uint64_t opt, Process* proc);

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

uint32_t sys_time_get_timebase_frequency(PPUThread* thread);
uint32_t sys_time_get_current_time(int64_t* sec, int64_t* nsec);
uint32_t sys_time_get_timezone(uint32_t* timezone, uint32_t* summertime);

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
int32_t sys_ppu_thread_exit(uint64_t code, PPUThread* thread);
int32_t sys_ppu_thread_set_priority(sys_ppu_thread_t thread_id, int32_t prio, Process* proc);
emu_void_t sys_ppu_thread_yield(PPUThread* thread);
int32_t sys_ppu_thread_get_priority(sys_ppu_thread_t thread_id, big_int32_t* prio, Process* proc);

emu_void_t sys_process_exit(PPUThread* thread);

int32_t _sys_strlen(cstring_ptr_t str);
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
                                 ps3_uintptr_t idlist,
                                 PPUThread* thread);
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

uint32_t emuEmptyModuleStart();
