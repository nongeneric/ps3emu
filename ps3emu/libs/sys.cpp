#include "ConcurrentQueue.h"
#include "ps3emu/ContentManager.h"
#include "../utils.h"
#include "sys.h"
#include <time.h>
#include <stdio.h>
#include <stdexcept>
#include "../Process.h"
#include "../IDMap.h"
#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/optional.hpp>
#include "../log.h"
#include <memory>
#include <map>

void init_sys_lib() {
    LOG << __FUNCTION__;
}

void sys_initialize_tls(uint64_t undef, uint32_t unk1, uint32_t unk2) {
    LOG << __FUNCTION__;
}

int sys_memory_get_user_memory_size(sys_memory_info_t* mem_info) {
    LOG << __FUNCTION__;
    mem_info->total_user_memory = 221249536;
    mem_info->available_user_memory = 0.9 * mem_info->total_user_memory; // TODO: handle alloc/dealloc
    return CELL_OK;
}

cell_system_time_t sys_time_get_system_time(PPUThread* thread) {
    LOG << __FUNCTION__;
    auto sec = (float)thread->proc()->getTimeBase() / (float)thread->proc()->getFrequency();
    return sec * 1000000;
}

int _sys_process_atexitspawn() {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int _sys_process_at_Exitspawn() {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int sys_ppu_thread_get_id(sys_ppu_thread_t* thread_id) {
    *thread_id = 7;
    return CELL_OK;
}

#define SYS_TTYP_MAX  (0x10)

#define SYS_TTYP0     (0)
#define SYS_TTYP1     (1)
#define SYS_TTYP2     (2)
#define SYS_TTYP3     (3)
#define SYS_TTYP4     (4)
#define SYS_TTYP5     (5)
#define SYS_TTYP6     (6)
#define SYS_TTYP7     (7)
#define SYS_TTYP8     (8)
#define SYS_TTYP9     (9)
#define SYS_TTYP10   (10)
#define SYS_TTYP11   (11)
#define SYS_TTYP12   (12)
#define SYS_TTYP13   (13)
#define SYS_TTYP14   (14)
#define SYS_TTYP15   (15)

#define  SYS_TTYP_PPU_STDIN    (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDOUT   (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDERR   (SYS_TTYP1)
#define  SYS_TTYP_SPU_STDOUT   (SYS_TTYP2)
#define  SYS_TTYP_USER1        (SYS_TTYP3)
#define  SYS_TTYP_USER2        (SYS_TTYP4)
#define  SYS_TTYP_USER3        (SYS_TTYP5)
#define  SYS_TTYP_USER4        (SYS_TTYP6)
#define  SYS_TTYP_USER5        (SYS_TTYP7)
#define  SYS_TTYP_USER6        (SYS_TTYP8)
#define  SYS_TTYP_USER7        (SYS_TTYP9)
#define  SYS_TTYP_USER8        (SYS_TTYP10)
#define  SYS_TTYP_USER9        (SYS_TTYP11)
#define  SYS_TTYP_USER10       (SYS_TTYP12)
#define  SYS_TTYP_USER11       (SYS_TTYP13)
#define  SYS_TTYP_USER12       (SYS_TTYP14)
#define  SYS_TTYP_USER13       (SYS_TTYP15)

int sys_tty_write(unsigned int ch, const void* buf, unsigned int len, unsigned int* pwritelen) {
    if (len == 0)
        return CELL_OK;
    if (ch == SYS_TTYP_PPU_STDOUT) {
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_PPU_STDERR) {
        fwrite(buf, 1, len, stderr);
        fflush(stderr);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_SPU_STDOUT) {
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        return CELL_OK;
    }
    throw std::runtime_error(ssnprintf("unknown channel %d", ch));
}

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int sys_prx_exitspawn_with_level(uint64_t level) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_prx_register_library(ps3_uintptr_t library) {
    return CELL_OK;
}

boost::optional<fdescr> findExport(MainMemory* mm, ELFLoader* prx, uint32_t eid) {
    prx_export_t* exports;
    int count;
    std::tie(exports, count) = prx->exports(mm);
    for (auto i = 0; i < count; ++i) {
        if (exports[i].name)
            continue;
        auto fnids = (big_uint32_t*)mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
        auto stubs = (big_uint32_t*)mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
        for (auto j = 0; j < exports[i].functions; ++j) {
            if (fnids[j] == eid) {
                fdescr descr;
                mm->readMemory(stubs[j], &descr, sizeof(descr));
                return descr;
            }
        }
    }
    return {};
}

int32_t executeExportedFunction(sys_prx_id_t id,
                                size_t args,
                                ps3_uintptr_t argp,
                                ps3_uintptr_t modres, // big_int32_t*
                                PPUThread* thread,
                                const char* name) {
    auto& segments = thread->proc()->getSegments();
    auto segment = boost::find_if(segments, [=](auto& s) { return s.va == id; });
    assert(segment != end(segments));
    auto func = findExport(thread->mm(), segment->elf.get(), calcEid(name));
    assert(func);
    thread->setGPR(2, func->tocBase);
    thread->setGPR(3, args);
    thread->setGPR(4, argp);
    thread->ps3call(func->va,
                    [=] { thread->mm()->store<4>(modres, thread->getGPR(3)); });
    return thread->getGPR(3);
}

int32_t sys_prx_start_module(sys_prx_id_t id,
                             size_t args,
                             ps3_uintptr_t argp,
                             ps3_uintptr_t modres, // big_int32_t*
                             uint64_t flags,
                             uint64_t pOpt,
                             PPUThread* thread) {
    assert(flags == 0);
    assert(pOpt == 0);
    return executeExportedFunction(id, args, argp, modres, thread, "module_start");
}

sys_prx_id_t sys_prx_load_module(cstring_ptr_t path, uint64_t flags, uint64_t opt, Process* proc) {
    assert(flags == 0);
    assert(opt == 0);
    LOG << ssnprintf("sys_prx_load_module(%s)", path.str);
    auto hostPath = proc->contentManager()->toHost(path.str.c_str()) + ".elf";
    return proc->loadPrx(hostPath);
}

int32_t sys_prx_stop_module(sys_prx_id_t id,
                            size_t args,
                            ps3_uintptr_t argp,
                            ps3_uintptr_t modres,
                            uint64_t flags,
                            uint64_t pOpt,
                            PPUThread* thread) {
    assert(flags == 0);
    assert(pOpt == 0);
    return executeExportedFunction(id, args, argp, modres, thread, "module_stop");
}

int32_t sys_prx_unload_module(sys_prx_id_t id, uint64_t flags, uint64_t pOpt) {
    assert(flags == 0);
    assert(pOpt == 0);
    WARNING(libs) << "sys_prx_unload_module not implemented";
    return CELL_OK;
}

constexpr uint32_t SYS_MEMORY_PAGE_SIZE_1M = 0x400;
constexpr uint32_t SYS_MEMORY_PAGE_SIZE_64K = 0x200;

int sys_memory_allocate(uint32_t size, uint64_t flags, sys_addr_t* alloc_addr, PPUThread* thread) {
    LOG << ssnprintf("sys_memory_allocate(%x,...)", size);
    (void)SYS_MEMORY_PAGE_SIZE_1M; (void)SYS_MEMORY_PAGE_SIZE_64K;
    assert(flags == SYS_MEMORY_PAGE_SIZE_1M || flags == SYS_MEMORY_PAGE_SIZE_64K);
    assert(size < 256 * 1024 * 1024);
    *alloc_addr = thread->mm()->malloc(size);
    return CELL_OK;
}

int sys_memory_free(ps3_uintptr_t start_addr, PPUThread* thread) {
    LOG << ssnprintf("sys_memory_free(%x)", start_addr);
    thread->mm()->free(start_addr);
    return CELL_OK;
}

int sys_timer_usleep(usecond_t sleep_time) {
    ums_sleep(sleep_time);
    return CELL_OK;
}

uint32_t sys_time_get_timebase_frequency(PPUThread* thread) {
    LOG << __FUNCTION__;
    return thread->proc()->getFrequency();
}

uint32_t sys_time_get_current_time(int64_t* sec, int64_t* nsec) {
    auto now = boost::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto ns = boost::chrono::duration_cast<boost::chrono::nanoseconds>(duration).count();
    *sec = ns / 1000000000;
    *nsec = ns % 1000000000;
    return CELL_OK;
}

uint32_t sys_time_get_timezone(uint32_t* timezone, uint32_t* summertime) {
    *timezone = 40; // us eastern time
    *summertime = 0;
    return CELL_OK;
}

int32_t sys_ppu_thread_get_stack_information(sys_ppu_thread_stack_t* info, PPUThread* thread) {
    LOG << __FUNCTION__;
    info->pst_addr = thread->getStackBase();
    info->pst_size = thread->getStackSize();
    return CELL_OK;
}

uint32_t cellSysmoduleLoadModule(uint16_t id) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

uint32_t cellSysmoduleUnloadModule(uint16_t id) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

uint32_t cellSysmoduleIsLoaded(uint16_t id) {
    return CELL_OK;
}

uint32_t cellSysmoduleInitialize() {
    return CELL_OK;
}

class CellSemaphore {
    boost::mutex _m;
    boost::condition_variable _cv;
    unsigned _val;
    unsigned _max;
public:
    CellSemaphore(unsigned val, unsigned max)
        : _val(val), _max(max) { }
        
    bool post(uint32_t val) {
        boost::unique_lock<boost::mutex> lock(_m);
        if (_val == _max)
            return false;
        _val += val;
        _cv.notify_all();
        return true;
    }
    
    bool wait(usecond_t timeout) {
        boost::unique_lock<boost::mutex> lock(_m);
        if (!lock.owns_lock())
            return false;
        while (!_val) {
            _cv.timed_wait(lock, boost::posix_time::microseconds(timeout));
        }
        _val--;
        return true;
    }
};

IDMap<sys_semaphore_t, std::unique_ptr<CellSemaphore>> semaphores;

int32_t sys_semaphore_create(sys_semaphore_t* sem, 
                             sys_semaphore_attribute_t* attr, 
                             sys_semaphore_value_t initial_val, 
                             sys_semaphore_value_t max_val) {
    auto csem = std::make_unique<CellSemaphore>(initial_val, max_val);
    *sem = semaphores.create(std::move(csem));
    return CELL_OK;
}

int32_t sys_semaphore_destroy(sys_semaphore_t sem) {
    semaphores.destroy(sem);
    return CELL_OK;
}

int32_t sys_semaphore_wait(sys_semaphore_t sem, usecond_t timeout) {
    const auto& csem = semaphores.get(sem);
    if (csem->wait(timeout))
        return CELL_OK;
    return CELL_ETIMEDOUT;
}

int32_t sys_semaphore_trywait(sys_semaphore_t sem) {
    return sys_semaphore_wait(sem, 0);
}

int32_t sys_semaphore_post(sys_semaphore_t sem, sys_semaphore_value_t val) {
    const auto& csem = semaphores.get(sem);
    if (csem->post(val))
        return CELL_OK;
    return CELL_EBUSY;
}

#define SYS_PPU_THREAD_CREATE_JOINABLE 0x0000000000000001
#define SYS_PPU_THREAD_CREATE_INTERRUPT 0x0000000000000002

int32_t sys_ppu_thread_create(sys_ppu_thread_t* thread_id,
                              ps3_uintptr_t entry,
                              uint64_t arg,
                              uint32_t prio,
                              uint32_t stacksize,
                              uint64_t flags,
                              const char *threadname,
                              Process* proc)
{
    assert(flags == 0 ||
           flags == SYS_PPU_THREAD_CREATE_JOINABLE ||
           flags == SYS_PPU_THREAD_CREATE_INTERRUPT);
    if (flags == SYS_PPU_THREAD_CREATE_INTERRUPT) {
        *thread_id = proc->createInterruptThread(stacksize, entry, arg);
    } else {
        *thread_id = proc->createThread(stacksize, entry, arg);
    }
    return CELL_OK;
}

int32_t sys_ppu_thread_join(sys_ppu_thread_t thread_id, big_uint64_t* exit_code, Process* proc) {
    auto thread = proc->getThread(thread_id);
    *exit_code = thread->join();
    return CELL_OK;
}

int32_t sys_ppu_thread_exit(uint64_t code, PPUThread* thread) {
    throw ThreadFinishedException(code);
}

emu_void_t sys_process_exit(PPUThread* thread) {
    throw ProcessFinishedException();
}

emu_void_t sys_initialize_tls(uint64_t undef, uint64_t unk1, uint64_t unk2, PPUThread* thread) {
    return emu_void;
}

int32_t sys_process_is_stack(ps3_uintptr_t p) {
    return StackArea <= p && p < StackArea + StackAreaSize;
}

emu_void_t sys_ppu_thread_yield(PPUThread* thread) {
    thread->yield();
    return emu_void;
}

int32_t sys_ppu_thread_get_priority(sys_ppu_thread_t thread_id, int32_t* prio, Process* proc) {
    auto thread = proc->getThread(thread_id);
    *prio = thread->priority();
    return CELL_OK;
}

int32_t sys_ppu_thread_set_priority(sys_ppu_thread_t thread_id, int32_t prio, Process* proc) {
    assert(0 <= prio && prio <= 3071);
    auto thread = proc->getThread(thread_id);
    thread->setPriority(prio);
    return CELL_OK;
}

int32_t _sys_strlen(cstring_ptr_t str) {
    return str.str.size();
}

int32_t sys_process_is_spu_lock_line_reservation_address(ps3_uintptr_t addr, uint64_t flags) {
    return 0;
}