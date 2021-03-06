#include "ConcurrentQueue.h"
#include "ps3emu/ContentManager.h"
#include "../utils.h"
#include "sys.h"
#include <time.h>
#include <stdio.h>
#include <stdexcept>
#include "../Process.h"
#include "../InternalMemoryManager.h"
#include "../ELFLoader.h"
#include "../IDMap.h"
#include "ps3emu/HeapMemoryAlloc.h"
#include "ps3emu/EmuCallbacks.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/range/algorithm.hpp>
#include "../log.h"
#include "../state.h"
#include "ps3emu/libs/fs.h"
#include <time.h>
#include <memory>
#include <map>

namespace {
    std::vector<SharedMemoryInfo> g_sharedMemoryInfos;
}

void init_sys_lib() {
    INFO(libs) << __FUNCTION__;
}

int sys_memory_get_user_memory_size(sys_memory_info_t* mem_info) {
    mem_info->total_user_memory = HeapAreaSize;
    mem_info->available_user_memory = g_state.heapalloc->available();
    INFO(libs) << sformat("sys_memory_get_user_memory_size({:x}, {:x})",
                            mem_info->total_user_memory,
                            mem_info->available_user_memory);
    return CELL_OK;
}

int _sys_process_atexitspawn() {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int _sys_process_at_Exitspawn() {
    INFO(libs) << __FUNCTION__;
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

int sys_tty_write(uint32_t ch,
                  uint32_t buf_va,
                  uint32_t buf_len,
                  uint32_t* pwritelen) {
    if (buf_len == 0)
        return CELL_OK;

    auto buf = g_state.mm->getMemoryPointer(buf_va, buf_len);
    if (ch == SYS_TTYP_PPU_STDOUT) {
        g_state.callbacks->stdout((char*)buf, buf_len);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_PPU_STDERR) {
        g_state.callbacks->stderr((char*)buf, buf_len);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_SPU_STDOUT) {
        g_state.callbacks->spustdout((char*)buf, buf_len);
        return CELL_OK;
    }
    throw std::runtime_error(sformat("unknown channel {}", ch));
}

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int sys_prx_exitspawn_with_level(uint64_t level) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

uint32_t emuEmptyModuleStart() {
    return CELL_OK;
}

int32_t sys_prx_start_module(sys_prx_id_t id,
                             uint64_t flags,
                             sys_prx_start_module_t* opt,
                             PPUThread* thread) {
    assert(flags == 0);
    assert(opt->struct_size == sizeof(sys_prx_start_module_t));
    assert(opt->neg1 == -1ull);
    assert(opt->mode == 1 || opt->mode == 2);
    if (opt->mode == 1) {
        INFO(libs) << sformat("sys_prx_start_module(find, {:08x}, {})",
                                id,
                                (char*)&opt->name_or_fdescrva);
        opt->name_or_fdescrva = findExportedModuleFunction(id, "module_start");
        if (!opt->name_or_fdescrva) {
            uint32_t descrva;
            auto descr = g_state.memalloc->internalAlloc<4, fdescr>(&descrva);
            descr->va = descrva + 4;
            uint32_t index;
            auto entry = findNCallEntry(calcFnid("emuEmptyModuleStart"), index);
            assert(entry); (void)entry;
            encodeNCall(g_state.mm, descrva + 4, index);
            opt->name_or_fdescrva = descrva;
        }
        return CELL_OK;
    } else {
        return CELL_OK;
    }
    assert(false);
    return 0;
}

sys_prx_id_t sys_prx_load_module(cstring_ptr_t path, uint64_t flags, uint64_t opt, Process* proc) {
    assert(flags == 0);
    assert(opt == 0);
    INFO(libs) << sformat("sys_prx_load_module({})", path.str);
    auto hostPath = g_state.content->toHost(path.str.c_str()) + ".elf";
    return proc->loadPrx(hostPath);
}

sys_prx_id_t sys_prx_load_module_on_memcontainer(
    cstring_ptr_t path,
    uint32_t mem_container,
    uint64_t flags,
    uint32_t pOpt)
{
    return sys_prx_load_module(path, flags, pOpt, g_state.proc);
}

int32_t sys_prx_stop_module(sys_prx_id_t id,
                            uint64_t flags,
                            sys_prx_start_module_t* opt,
                            PPUThread* thread) {
    assert(flags == 0);
    assert(opt->struct_size == sizeof(sys_prx_start_module_t));
    assert(opt->neg1 == -1ull);
    auto mode = enum_cast<prx_module_mode>(opt->mode);

    if (mode == prx_module_mode::user_lookup || mode == prx_module_mode::system_lookup) {
        opt->name_or_fdescrva = id == 0 ? 0 : findExportedModuleFunction(id, "module_stop");
        if (!opt->name_or_fdescrva) {
            uint32_t descrva;
            auto descr = g_state.memalloc->internalAlloc<4, fdescr>(&descrva);
            descr->va = descrva + 4;
            uint32_t index;
            auto entry = findNCallEntry(calcFnid("emuEmptyModuleStart"), index);
            assert(entry); (void)entry;
            encodeNCall(g_state.mm, descrva + 4, index);
            opt->name_or_fdescrva = descrva;
        }
    } else if (mode == prx_module_mode::user_confirm || mode == prx_module_mode::system_confirm) {
    }

    INFO(libs) << sformat("sys_prx_stop_module(find, {:08x}, {}, mode: {})",
                            id,
                            (char*)&opt->name_or_fdescrva,
                            to_string(mode));

    return CELL_OK;
}

int32_t sys_prx_unload_module(sys_prx_id_t id, uint64_t flags, uint64_t pOpt) {
    assert(flags == 0);
    assert(pOpt == 0);
    auto segments = g_state.proc->getSegments();
    auto it = std::find_if(begin(segments), end(segments), [&](auto& s) {
        return s.va == id;
    });
    if (it == end(segments))
        throw std::runtime_error("sys_prx_unload_module failed");
    auto index = std::distance(begin(segments), it);
    g_state.proc->unloadSegment(segments[index].va);
    g_state.proc->unloadSegment(segments[index + 1].va);
    return CELL_OK;
}

constexpr uint32_t SYS_MEMORY_PAGE_SIZE_1M = 0x400;
constexpr uint32_t SYS_MEMORY_PAGE_SIZE_64K = 0x200;

int sys_memory_allocate(uint32_t size, uint64_t flags, sys_addr_t* alloc_addr, PPUThread* thread) {
    INFO(libs) << sformat("sys_memory_allocate({:x},...)", size);
    (void)SYS_MEMORY_PAGE_SIZE_1M; (void)SYS_MEMORY_PAGE_SIZE_64K;
    assert(flags == SYS_MEMORY_PAGE_SIZE_1M || flags == SYS_MEMORY_PAGE_SIZE_64K);
    assert(size < 256 * 1024 * 1024);
    auto alignment = SYS_MEMORY_PAGE_SIZE_1M ? (1ul << 20) : (64u << 10);
    auto [ptr, va] = g_state.heapalloc->alloc(size, alignment);
    if (!ptr)
        return CELL_ENOMEM;
    *alloc_addr = va;
    return CELL_OK;
}

int sys_memory_free(ps3_uintptr_t start_addr, PPUThread* thread) {
    INFO(libs) << sformat("sys_memory_free({:x})", start_addr);
    g_state.heapalloc->dealloc(start_addr);
    return CELL_OK;
}

int sys_timer_usleep(usecond_t sleep_time) {
    ums_sleep(sleep_time);
    return CELL_OK;
}

int sys_timer_sleep(second_t sleep_time) {
    ums_sleep(sleep_time * 1000000ull);
    return CELL_OK;
}

uint32_t sys_time_get_timebase_frequency(PPUThread* thread) {
    INFO(libs) << __FUNCTION__;
    return g_state.proc->getFrequency();
}

uint32_t sys_time_get_current_time(big_int64_t* sec, big_int64_t* nsec) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    *sec = ts.tv_sec;
    *nsec = ts.tv_nsec;
    return CELL_OK;
}

uint32_t sys_time_get_timezone(big_uint32_t* timezone, big_uint32_t* summertime) {
    *timezone = 40; // us eastern time
    *summertime = 0;
    return CELL_OK;
}

int32_t sys_ppu_thread_get_stack_information(sys_ppu_thread_stack_t* info, PPUThread* thread) {
    INFO(libs) << __FUNCTION__;
    info->pst_addr = thread->getStackBase();
    info->pst_size = thread->getStackSize();
    return CELL_OK;
}

uint32_t cellSysmoduleLoadModule(uint16_t id) {
    INFO(libs) << sformat("cellSysmoduleLoadModule(0x{:x})", id);
    return CELL_OK;
}

uint32_t cellSysmoduleUnloadModule(uint16_t id) {
    INFO(libs) << __FUNCTION__;
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
        auto lock = boost::unique_lock(_m);
        if (_val == _max)
            return false;
        _val += val;
        _cv.notify_all();
        return true;
    }

    bool wait(usecond_t timeout) {
        auto lock = boost::unique_lock(_m);
        if (!lock.owns_lock())
            return false;
        while (!_val) {
            _cv.timed_wait(lock, boost::posix_time::microseconds(static_cast<uint64_t>(timeout)));
        }
        _val--;
        return true;
    }

    unsigned value() {
        auto lock = boost::unique_lock(_m);
        return _val;
    }
};

IDMap<sys_semaphore_t, std::unique_ptr<CellSemaphore>> semaphores;

int32_t sys_semaphore_create(sys_semaphore_t* sem,
                             sys_semaphore_attribute_t* attr,
                             sys_semaphore_value_t initial_val,
                             sys_semaphore_value_t max_val) {
    auto csem = std::make_unique<CellSemaphore>(initial_val, max_val);
    *sem = semaphores.create(std::move(csem));
    INFO(libs) << sformat("sys_semaphore_create({})", *sem);
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

int32_t sys_semaphore_get_value(sys_semaphore_t sem, sys_semaphore_value_t* val) {
    const auto& csem = semaphores.get(sem);
    *val = csem->value();
    return CELL_OK;
}

#define SYS_PPU_THREAD_CREATE_JOINABLE 0x0000000000000001
#define SYS_PPU_THREAD_CREATE_INTERRUPT 0x0000000000000002

int32_t sys_ppu_thread_create(sys_ppu_thread_t* thread_id,
                              const ppu_thread_create_t* info,
                              uint64_t arg,
                              uint64_t unk,
                              uint32_t prio,
                              uint32_t stacksize,
                              uint64_t flags,
                              cstring_ptr_t threadname)
{
    assert(unk == 0);
    assert(flags == 0 ||
           flags == SYS_PPU_THREAD_CREATE_JOINABLE ||
           flags == SYS_PPU_THREAD_CREATE_INTERRUPT);
    if (flags == SYS_PPU_THREAD_CREATE_INTERRUPT) {
        *thread_id = g_state.proc->createInterruptThread(
            stacksize, info->entry_fdescr_va, arg, threadname.str, info->tls_va, true);
    } else {
        *thread_id = g_state.proc->createThread(
            stacksize, info->entry_fdescr_va, arg, threadname.str, info->tls_va, false);
    }
    return CELL_OK;
}

int32_t sys_ppu_thread_start(sys_ppu_thread_t id) {
    INFO(libs) << sformat("sys_ppu_thread_start({})", id);
    auto thread = g_state.proc->getThread(id);
    thread->run();
    return CELL_OK;
}

int32_t sys_ppu_thread_join(sys_ppu_thread_t thread_id, big_uint64_t* exit_code, Process* proc) {
    auto thread = proc->getThread(thread_id);
    *exit_code = thread->join();
    return CELL_OK;
}

int32_t sys_ppu_thread_detach(sys_ppu_thread_t thread_id) {
    WARNING(libs) << "sys_ppu_thread_detach not implemented";
    return CELL_OK;
}

int32_t sys_ppu_thread_exit(uint64_t code, PPUThread* thread) {
    throw ThreadFinishedException(code);
}

emu_void_t sys_process_exit(int32_t status) {
    throw ProcessFinishedException(status);
}

emu_void_t sys_ppu_thread_yield(PPUThread* thread) {
    thread->yield();
    return emu_void;
}

int32_t sys_ppu_thread_get_priority(sys_ppu_thread_t thread_id, big_int32_t* prio, Process* proc) {
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

int32_t sys_process_is_spu_lock_line_reservation_address(ps3_uintptr_t addr, uint64_t flags) {
    return CELL_OK;
}

uint32_t sys_process_getpid() {
    return 0x01000500;
}

int32_t sys_process_get_sdk_version(uint32_t pid, big_uint32_t* version) {
    assert(pid == 0x01000500);
    *version = 0x00400001;
    return CELL_OK;
}

sys_prx_id_t sys_prx_get_module_id_by_name(cstring_ptr_t name,
                                           uint64_t flags,
                                           uint64_t pOpt,
                                           PPUThread* thread) {
    assert(!flags);
    assert(!pOpt);
    if (name.str == "cellLibprof") {
        uint32_t CELL_PRX_ERROR_UNKNOWN_MODULE = 0x8001112e;
        return CELL_PRX_ERROR_UNKNOWN_MODULE; // not a tuner
    }
    for (auto& s : g_state.proc->getSegments()) {
        prx_export_t* exports;
        int nexports;
        std::tie(exports, nexports) = s.elf->exports();
        for (auto i = 0; i < nexports; ++i) {
            if (exports[i].name) {
                std::string exportName;
                readString(g_state.mm, exports[i].name, exportName);
                if (name.str == exportName)
                    return s.va;
            }
        }
    }
    return -1;
}

int32_t _sys_printf(cstring_ptr_t format, PPUThread* thread) {
    std::string str;
    auto arg = 4;
    for (auto i = 0u; i < format.str.size(); ++i) {
        auto ch = format.str[i];
        if (ch == '%') {
            i++;
            ch = format.str[i];
            if (ch == 'x') {
                str += sformat("{:x}", thread->getGPR(arg));
                arg++;
            } else if (ch == 'f') {
                str += sformat("{}", bit_cast<double>(thread->getGPR(arg)));
                arg++;
            } else if (ch == 'u') {
                str += sformat("{}", thread->getGPR(arg));
                arg++;
            } else if (ch == 'd') {
                str += sformat("{}", static_cast<int64_t>(thread->getGPR(arg)));
                arg++;
            } else if (ch == 's') {
                std::string substr;
                readString(g_state.mm, thread->getGPR(arg), substr);
                str += substr;
                arg++;
            } else {
                ERROR(libs) << "bad _sys_printf format";
                break;
            }
        } else {
            str += ch;
        }
    }
    puts(str.c_str());
    return CELL_OK;
}

int32_t _sys_process_get_paramsfo(std::array<char, 0x40>* sfo) {
    memset(&(*sfo)[0], 0, sfo->size());
    (*sfo)[0] = 1;
    for (auto& p : g_state.content->sfo()) {
        if (p.id != CELL_GAME_PARAMID_TITLE_DEFAULT)
            continue;
        strcpy(&(*sfo)[1], boost::get<std::string>(p.data).c_str());
    }
    return CELL_OK;
}

int32_t sys_get_process_info(process_info_t* info) {
    assert(info->size == 0x50);
    WARNING(libs) << "sys_get_process_info not implemented";
    return CELL_OK;
}

int32_t sys_prx_register_module(cstring_ptr_t name, uint64_t opts) {
    INFO(libs) << sformat("sys_prx_register_module({})", name.str);
    return CELL_OK;
}

int32_t sys_prx_register_library(uint32_t va) {
    INFO(libs) << sformat("sys_prx_register_library({:08x})", va);
    return CELL_OK;
}

int32_t sys_ss_access_control_engine(uint64_t unk1, uint64_t unk2, uint64_t unk3) {
    WARNING(libs) << "sys_ss_access_control_engine not implemented";
    return CELL_OK;
}

int32_t sys_prx_load_module_list(int32_t n,
                                 ps3_uintptr_t path_list_va,
                                 uint64_t flags,
                                 uint64_t pOpt,
                                 ps3_uintptr_t idlist) {
    INFO(libs) << sformat("sys_prx_load_module_list({}, ...)", n);
    assert(flags == 0);
    for (auto i = 0; i < n; ++i) {
        auto pathVa = g_state.mm->load64(path_list_va);
        std::string path;
        readString(g_state.mm, pathVa, path);
        uint32_t id = sys_prx_load_module({path}, flags, pOpt, g_state.proc);
        g_state.mm->store32(idlist, id, g_state.granule);
        path_list_va += 8;
        idlist += 4;
    }
    return CELL_OK;
}

int32_t sys_mmapper_allocate_shared_memory(uint64_t id,
                                           uint32_t size,
                                           uint32_t alignment,
                                           big_uint32_t* mem) {
    if (emuFindSharedMemoryInfo(id))
        return CELL_EEXIST;
    alignment = 1u << (31 - __builtin_clz(alignment));
    uint32_t va;
    g_state.memalloc->allocInternalMemory(&va, size, alignment);
    *mem = va;
    INFO(libs) << sformat(
        "sys_mmapper_allocate_shared_memory({:x}, {:08x}, {:02x}, va={:x})",
        id,
        size,
        alignment,
        va);
    g_sharedMemoryInfos.push_back({id, size, va});
    return CELL_OK;
}

int32_t sys_mmapper_allocate_address(uint32_t size,
                                     uint64_t flags,
                                     uint32_t alignment,
                                     big_uint32_t* addr) {
    INFO(libs) << sformat(
        "sys_mmapper_allocate_address({:08x}, {:08x}, {:02x})", size, flags, alignment);
    *addr = 0xbadbad10;
    return CELL_OK;
}

int32_t sys_mmapper_search_and_map(uint32_t start_addr,
                                   uint32_t mem_id,
                                   uint64_t flags,
                                   big_uint32_t* alloc_addr) {
    INFO(libs) << sformat("sys_mmapper_search_and_map({:08x}, {:08x}, {:02x})",
                            start_addr,
                            mem_id,
                            flags);
    assert(start_addr == 0xbadbad10);
    *alloc_addr = mem_id;
    return CELL_OK;
}

int32_t sys_mmapper_unmap_shared_memory(sys_addr_t start_addr, uint32_t mem_id) {
    INFO(libs) << sformat(
        "sys_mmapper_unmap_shared_memory({:08x}, {:08x})", start_addr, mem_id);
    return CELL_OK;
}

int32_t sys_mmapper_free_shared_memory(sys_addr_t start_addr) {
    INFO(libs) << sformat("sys_mmapper_free_shared_memory({:08x})", start_addr);
    g_state.memalloc->free(start_addr);
    return CELL_OK;
}

#define SYS_MODULE_STOP_LEVEL_USER	0x00000000
#define SYS_MODULE_STOP_LEVEL_SYSTEM	0x00004000

int32_t sys_prx_get_module_list(uint32_t flags, sys_prx_get_module_list_t* info) {
    INFO(libs) << sformat("sys_prx_get_module_list({:x}, ...)", flags);
    assert(flags == 2);
    auto& segments = g_state.proc->getSegments();
    assert(info->max_ids_size >= segments.size() / 2);
    info->out_count = segments.size() / 2;
    if (info->max_levels_size) {
        WARNING(libs) << "sys_prx_get_module_list doesn't assign levels correctly";
    }
    for (auto i = 0u; i < segments.size() / 2 - 1; ++i) {
        auto& segment = segments.at((i + 1) * 2);
        g_state.mm->store32(info->ids_va + 4 * i, segment.va, g_state.granule);
        if (i < info->max_levels_size) {
            g_state.mm->store32(info->levels_va + 4 * i, SYS_MODULE_STOP_LEVEL_SYSTEM, g_state.granule);
        }
    }
    return CELL_OK;
}

int sys_memory_get_page_attribute(uint32_t addr, sys_page_attr_t* attr) {
    WARNING(libs) << "sys_memory_get_page_attribute is only partially implemented";
    attr->attribute = SYS_MEMORY_PROT_READ_WRITE;
    attr->page_size = 1u << 20u;
    attr->access_right = SYS_MEMORY_ACCESS_RIGHT_ANY;
    return CELL_OK;
}

emu_void_t _sys_memcpy(uint64_t dest, uint64_t src, uint64_t size, MainMemory* mm) {
    auto srcptr = mm->getMemoryPointer(src, size);
    mm->writeMemory(dest, srcptr, size);
    return emu_void;
}

emu_void_t _sys_memset(uint64_t dest, uint64_t value, uint64_t size, MainMemory* mm) {
    mm->setMemory(dest, value, size);
    return emu_void;
}

std::optional<SharedMemoryInfo> emuFindSharedMemoryInfo(uint64_t id) {
    auto it = std::find_if(begin(g_sharedMemoryInfos),
                           end(g_sharedMemoryInfos),
                           [&](auto& info) { return info.id == id; });
    if (it == end(g_sharedMemoryInfos))
        return {};
    return *it;
}
