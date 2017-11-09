#include "sysSpu.h"

#include "SpuImage.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/ppu/InterruptPPUThread.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/utils.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/spu/SPUChannels.h"
#include <boost/range/algorithm.hpp>
#include <array>
#include <vector>
#include <stdio.h>
#include <assert.h>

struct InterruptTag;

struct RawSpu {
    std::shared_ptr<SPUThread> thread;
    std::shared_ptr<InterruptTag> tag;
};

struct InterruptTag {
    TagClassId classId;
    std::shared_ptr<RawSpu> rawSpu;
    InterruptPPUThread* interruptThread;
};

namespace {
    ThreadSafeIDMap<sys_spu_thread_group_t, std::shared_ptr<ThreadGroup>> groups;
    ThreadSafeIDMap<sys_raw_spu_t, std::shared_ptr<RawSpu>, 0> rawSpus;
    ThreadSafeIDMap<sys_interrupt_tag_t, std::shared_ptr<InterruptTag>> interruptTags;
    struct {
        uint32_t bufferVa = 0;
        char* buffer = nullptr;
        bool enabled = false;
    } spursTrace;
    constexpr uint32_t spursTraceBufferSize = 0x10000;
}

std::vector<sys_spu_segment_t> spuImageInit(MainMemory* mm,
                                            InternalMemoryManager* ialloc,
                                            sys_spu_image_t* image,
                                            ps3_uintptr_t elf,
                                            bool storeSegments) {
    Elf32_be_Ehdr header;
    mm->readMemory(elf, &header, sizeof(header));

    image->entry_point = header.e_entry;
    // memory is allocated by emu, this prevents lv2 from freeing it
    image->type = SYS_SPU_IMAGE_TYPE_KERNEL;

    assert(sizeof(Elf32_Phdr) == header.e_phentsize);
    std::vector<Elf32_be_Phdr> phs(header.e_phnum);
    std::vector<sys_spu_segment_t> segs;
    mm->readMemory(elf + header.e_phoff, &phs[0], phs.size() * sizeof(Elf32_Phdr));

    for (auto& ph : phs) {
        assert(ph.p_type != PT_LOAD || ph.p_type != PT_NOTE);
        segs.resize(segs.size() + 1);
        sys_spu_segment_t& seg = segs.back();
        seg.size = ph.p_filesz;
        seg.ls_start = ph.p_vaddr;
        seg.src.pa_start = ph.p_offset + elf;
        seg.type = ph.p_type == PT_LOAD ? SYS_SPU_SEGMENT_TYPE_COPY : SYS_SPU_SEGMENT_TYPE_INFO;
        
        if (ph.p_type == SYS_SPU_SEGMENT_TYPE_INFO) {
            uint32_t pos = ph.p_offset + elf;
            auto namesz = mm->load32(pos);
            pos += 4;
            auto descsz = mm->load32(pos);
            pos += 4;
            auto type = mm->load32(pos);
            pos += 4;
            assert(type == 1); (void)type;
            std::string name, desc;
            readString(mm, pos, name);
            readString(mm, pos + namesz, desc);
            assert(name == "SPUNAME");
            assert(12 + namesz + descsz == ph.p_filesz); (void)descsz;
            INFO(libs) << ssnprintf("initialized spu image SPUNAME = %s", desc);
            seg.size -= 12 + namesz;
            seg.src.pa_start += 12 + namesz;
        } else if (ph.p_memsz != ph.p_filesz) {
            assert(ph.p_memsz > ph.p_filesz);
            sys_spu_segment_t nextSeg;
            nextSeg.size = ph.p_memsz - ph.p_filesz;
            nextSeg.ls_start = seg.ls_start + seg.size;
            nextSeg.type = SYS_SPU_SEGMENT_TYPE_FILL;
            nextSeg.src.value = 0;
            segs.push_back(nextSeg);
        }
    }
    
    if (storeSegments) {
        uint32_t segsVa;
        auto size = segs.size() * sizeof(sys_spu_segment_t);
        auto isegs = ialloc->allocInternalMemory(&segsVa, size, 16);
        memcpy(isegs, &segs[0], size);
        image->segs = segsVa;
        image->nsegs = segs.size();
    }
    return segs;
}

void spuImageMap(MainMemory* mm, const sys_spu_image_t* image, void* ls) {
    auto segs = (sys_spu_segment_t*)mm -> getMemoryPointer(
                    image->segs, sizeof(sys_spu_segment_t) * image->nsegs);
    for (auto i = 0; i < image->nsegs; ++i) {
        auto& seg = segs[i];
        if (seg.type == SYS_SPU_SEGMENT_TYPE_COPY) {
            mm->readMemory(seg.src.pa_start, (char*)ls + seg.ls_start, seg.size);
        } else if (seg.type == SYS_SPU_SEGMENT_TYPE_FILL) {
            memset((char*)ls + seg.ls_start, seg.src.value, seg.size);
        }
    }
}

int32_t sys_spu_thread_read_ls(sys_spu_thread_t id,
                               uint32_t address,
                               uint64_t value,
                               size_t type) {
    INFO(libs) << __FUNCTION__;
    auto thread = g_state.proc->getSpuThread(id);
    big_uint64_t v = 0;
    memcpy((char*)&v + 8 - type, thread->ptr(address), type);
    g_state.mm->store64(value, v, g_state.granule);
    return CELL_OK;
}

int32_t sys_spu_thread_write_snr(sys_spu_thread_t id, int32_t number, uint32_t value) {
    INFO(libs) << ssnprintf("sys_spu_thread_write_snr(id=%x, number=%d, value=%x)", id, number, value);
    auto thread = g_state.proc->getSpuThread(id);
    auto ch = number ? SPU_Sig_Notify_2 : SPU_Sig_Notify_1;
    thread->channels()->mmio_write(ch, value);
    return CELL_OK;
}

int32_t sys_spu_initialize(uint32_t max_usable_spu, uint32_t max_raw_spu) {
    INFO(libs) << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_spu_image_import(sys_spu_image_t* img,
                             ps3_uintptr_t src,
                             uint32_t size,
                             uint32_t flags) {
    INFO(libs) << __FUNCTION__;
    EMU_ASSERT(flags == 0);
    
    uint32_t elfVa;
    auto elf = g_state.memalloc->allocInternalMemory(&elfVa, size, 16);
    g_state.mm->readMemory(src, elf, size, true);
    
#if DEBUG
    FILE* f = fopen("/tmp/ps3emu_lastSpuImage.elf", "w");
    assert(f);
    fwrite(elf, 1, size, f);
    fclose(f);
#endif
    
    img->type = SYS_SPU_IMAGE_TYPE_KERNEL;
    img->nsegs = 0;
    img->segs = 0;
    img->entry_point = elfVa;
    
    return CELL_OK;
}

int32_t sys_spu_image_open(sys_spu_image_t* img, cstring_ptr_t path) {
    INFO(libs) << ssnprintf("sys_spu_image_open(\"%s\")", path.str);
    auto hostPath = g_state.content->toHost(path.str.c_str());
    auto elfPath = hostPath;
    if (hostPath.substr(hostPath.size() - 4) != ".elf") {
        elfPath += ".elf";
    }
    FILE* f = fopen(elfPath.c_str(), "r");
    if (!f) {
        auto message = ssnprintf("sys_spu_image_open: elf not found (\"%s\")", elfPath);
        INFO(libs) << message;
        throw std::runtime_error(message);
    }
    
    fseek(f, 0, SEEK_END);
    uint32_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint32_t elfVa;
    auto elf = g_state.memalloc->allocInternalMemory(&elfVa, fsize, 16);
    fread((char*)elf, 1, fsize, f);
    fclose(f);
    
    img->type = SYS_SPU_IMAGE_TYPE_KERNEL;
    img->nsegs = 0;
    img->segs = 0;
    img->entry_point = elfVa;
    
    return CELL_OK;
}

int32_t sys_spu_image_get_info(const sys_spu_image_t *img, big_uint32_t* entry_point, big_uint32_t* nsegs) {
    sys_spu_image_t image_copy = *img;
    auto segs = spuImageInit(g_state.mm, g_state.memalloc, &image_copy, image_copy.entry_point, false);
    *nsegs = segs.size();
    *entry_point = image_copy.entry_point;
    return CELL_OK;
}

int32_t sys_spu_image_get_modules(const sys_spu_image_t *img, ps3_uintptr_t buf, uint32_t nsegs) {
    sys_spu_image_t image_copy = *img;
    auto segs = spuImageInit(g_state.mm, g_state.memalloc, &image_copy, image_copy.entry_point, false);
    assert(nsegs == segs.size());
    g_state.mm->writeMemory(buf, &segs[0], segs.size() * sizeof(sys_spu_segment_t));
    return CELL_OK;
}

int32_t sys_spu_image_close(sys_spu_image_t* img) {
    g_state.memalloc->free(img->entry_point);
    return CELL_OK;
}

int32_t sys_spu_thread_group_suspend(sys_spu_thread_group_t id) {
    auto group = groups.get(id);
    return g_state.spuGroupManager->suspend(group.get());
}

int32_t sys_spu_thread_group_resume(sys_spu_thread_group_t id) {
    auto group = groups.get(id);
    return g_state.spuGroupManager->resume(group.get());
}

int32_t sys_spu_thread_group_create(sys_spu_thread_group_t* id,
                                    uint32_t num,
                                    int32_t prio,
                                    sys_spu_thread_group_attribute_t* attr,
                                    MainMemory* mm) {
    INFO(libs) << __FUNCTION__;
    //assert(attr->type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL);
    auto group = std::make_shared<ThreadGroup>();
    group->name.resize(attr->nsize);
    group->priority = prio;
    mm->readMemory(attr->name, &group->name[0], attr->nsize);
    *id = groups.create(group);
    group->id = *id;
    g_state.spuGroupManager->add(group.get());
    return CELL_OK;
}

void initThread(MainMemory* mm, SPUThread* thread, const sys_spu_image_t* image) {
    spuImageMap(mm, image, thread->ptr(0));
    thread->setNip(image->entry_point);
    thread->setElfSource(image->segs);
}

int32_t sys_spu_thread_initialize(sys_spu_thread_t* thread_id,
                                  sys_spu_thread_group_t group_id,
                                  uint32_t spu_num,
                                  sys_spu_image_t* img,
                                  const sys_spu_thread_attribute_t* attr,
                                  const sys_spu_thread_argument_t* arg,
                                  Process* proc) {
    sys_spu_image_t image_copy = *img;
    if (img->type == SYS_SPU_IMAGE_TYPE_KERNEL) {
        img = &image_copy;
        spuImageInit(g_state.mm, g_state.memalloc, img, img->entry_point, true);
    }
    
    INFO(libs) << ssnprintf("sys_spu_thread_initialize() source=%x", img->segs);
    auto group = groups.get(group_id);
    std::string name;
    name.resize(attr->nsize);
    g_state.mm->readMemory(attr->name, &name[0], attr->nsize);
    auto id = proc->createSpuThread(name);
    auto th = proc->getSpuThread(id).get();
    group->threads.push_back(th);
    *thread_id = id;
    auto argcopy = *arg;
    auto imgcopy = *img;
    group->initializers[th] = [=] {
        th->setSpu(spu_num);
        initThread(g_state.mm, th, &imgcopy);
        th->r(3).set_dw(0, argcopy.arg1);
        th->r(4).set_dw(0, argcopy.arg2);
        th->r(5).set_dw(0, argcopy.arg3);
        th->r(6).set_dw(0, argcopy.arg4);
    };
    th->setGroup(group.get());
    return CELL_OK;
}

int32_t sys_spu_thread_group_start(sys_spu_thread_group_t id, Process* proc) {
    auto group = groups.get(id);
    INFO(libs) << ssnprintf("sys_spu_thread_group_start(%s)", group->name);
    g_state.spuGroupManager->start(group.get());
    return CELL_OK;
}

int32_t sys_spu_thread_group_join(sys_spu_thread_group_t gid,
                                  big_int32_t* cause,
                                  big_int32_t* status,
                                  Process* proc) {
    auto group = groups.get(gid);
    INFO(libs) << ssnprintf("sys_spu_thread_group_join(%s)", group->name);
    auto [c, s] = g_state.spuGroupManager->join(group.get());
    *cause = c;
    *status = s;
    return CELL_OK;
}

int32_t sys_spu_thread_group_destroy(sys_spu_thread_group_t id, Process* proc) {
    auto group = groups.get(id);
    g_state.spuGroupManager->destroy(group.get());
    groups.destroy(id);
    return CELL_OK;
}

int32_t sys_spu_thread_get_exit_status(sys_spu_thread_t id,
                                       big_int32_t* status,
                                       Process* proc) {
    auto th = g_state.proc->getSpuThread(id);
    for (auto& groupPair : groups.map()) {
        for (auto& threadPair : groupPair.second->errorCodes) {
            if (threadPair.first == th.get()) {
                *status = threadPair.second;
                return CELL_OK;
            }
        }
    }
    throw std::runtime_error("requesting error code of unknown spu thread");
}

int32_t sys_raw_spu_create(sys_raw_spu_t* id, uint32_t, Process* proc) {
    auto rawSpu = std::make_shared<RawSpu>();
    *id = rawSpus.create(rawSpu);
    rawSpu->thread = proc->getSpuThread(proc->createSpuThread("raw spu thread"));
    return CELL_OK;
}

int32_t sys_raw_spu_destroy(sys_raw_spu_t id, Process* proc) {
    auto rawSpu = rawSpus.get(id);
    auto tag = rawSpu->tag;
    if (tag) {
        tag->interruptThread->disestablish();
    }
    static int hangingRawSpuThreads = 0;
    if (rawSpu->thread->tryJoin(200).cause == SPUThreadExitCause::StillRunning) {
        // leave a hanging thread if couldn't join
        assert(hangingRawSpuThreads < 10);
        rawSpus.destroy(id);
        hangingRawSpuThreads++;
    }
    return CELL_OK;
}

int32_t sys_raw_spu_image_load(sys_raw_spu_t id, sys_spu_image_t* img, PPUThread* th) {
    auto rawSpu = rawSpus.get(id);
    initThread(g_state.mm, rawSpu->thread.get(), img);
    return CELL_OK;
}

int32_t sys_raw_spu_load(sys_raw_spu_t id, cstring_ptr_t path, big_uint32_t* entry, Process* proc) {
    auto rawSpu = rawSpus.get(id);
    sys_spu_image_t img;
    sys_spu_image_open(&img, path);
    *entry = img.entry_point;
    initThread(g_state.mm, rawSpu->thread.get(), &img);
    return CELL_OK;
}

int32_t sys_raw_spu_create_interrupt_tag(sys_raw_spu_t id,
                                         unsigned class_id,
                                         uint32_t unused,
                                         sys_interrupt_tag_t* intrtag) {
    INFO(libs) << __FUNCTION__;
    auto tag = std::make_shared<InterruptTag>();
    tag->classId = (TagClassId)class_id;
    tag->rawSpu = rawSpus.get(id);
    tag->rawSpu->tag = tag;
    *intrtag = interruptTags.create(tag);
    return CELL_OK;
}

int32_t sys_interrupt_thread_establish(sys_interrupt_thread_handle_t* ih,
                                       sys_interrupt_tag_t intrtag,
                                       uint32_t intrthread,
                                       uint64_t arg) {
    INFO(libs) << __FUNCTION__;
    auto th = dynamic_cast<InterruptPPUThread*>(g_state.proc->getThread(intrthread));
    auto tag = interruptTags.get(intrtag);
    tag->interruptThread = th;
    th->setArg(arg);
    th->establish(tag->rawSpu->thread.get());
    return CELL_OK;
}

int32_t sys_interrupt_thread_disestablish(sys_interrupt_thread_handle_t ih) {
    WARNING(libs) << "sys_interrupt_thread_disestablish not implemented";
    return CELL_OK;
}

int32_t sys_interrupt_tag_destroy(sys_interrupt_tag_t intrtag) {
    WARNING(libs) << "sys_interrupt_tag_destroy not implemented";
    return CELL_OK;
}

int32_t sys_raw_spu_set_int_mask(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 uint64_t mask) {
    INFO(libs) << __FUNCTION__;
    assert(class_id == 2);
    auto rawSpu = rawSpus.get(id);
    auto interruptThread = rawSpu->tag->interruptThread;
    interruptThread->setMask2(mask);
    return CELL_OK;
}

int32_t sys_raw_spu_get_int_stat(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 big_uint64_t* stat) {
    if (class_id != 2) {
        WARNING(libs) << "sys_raw_spu_get_int_stat: unimplemented spu interrupt class";
    }
    auto rawSpu = rawSpus.get(id);
    // only privileged software
    *stat = rawSpu->thread->channels()->interrupt();
    return CELL_OK;
}

int32_t sys_raw_spu_read_puint_mb(sys_raw_spu_t id, big_uint32_t* value) {
    auto rawSpu = rawSpus.get(id);
    *value = rawSpu->thread->channels()->mmio_read(SPU_Out_Intr_Mbox);
    return CELL_OK;
}

int32_t sys_spu_thread_write_spu_mb(uint32_t thread_id, uint32_t value) {
    auto thread = g_state.proc->getSpuThread(thread_id);
    thread->channels()->mmio_write(SPU_In_MBox, value);
    return CELL_OK;
}

int32_t sys_raw_spu_set_int_stat(sys_raw_spu_t id, uint32_t class_id, uint64_t stat) {
    if (class_id != 2) {
        WARNING(libs) << "sys_raw_spu_set_int_stat: unimplemented spu interrupt class";
    }
    auto rawSpu = rawSpus.get(id);
    // only privileged software
    rawSpu->thread->channels()->interrupt() = stat;
    return CELL_OK;
}

emu_void_t sys_interrupt_thread_eoi() {
    throw ThreadFinishedException(0);
}

std::shared_ptr<SPUThread> findRawSpuThread(sys_raw_spu_t id) {
    return rawSpus.get(id)->thread;
}

std::shared_ptr<ThreadGroup> findThreadGroup(sys_spu_thread_group_t id) {
    return groups.get(id);
}

#define SYS_SPU_THREAD_GROUP_EVENT_RUN               0x1
#define SYS_SPU_THREAD_GROUP_EVENT_RUN_KEY           0xFFFFFFFF53505500ULL
#define SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION         0x2
#define SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION_KEY     0xFFFFFFFF53505503ULL

int32_t sys_spu_thread_group_connect_event(sys_spu_thread_group_t id,
                                           sys_event_queue_t eq,
                                           sys_event_type_t et) {
    // emu doesn't expect exceptions
    assert(et == SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
    return CELL_OK;
}

int32_t sys_spu_thread_group_disconnect_event(sys_spu_thread_group_t id,
                                              sys_event_queue_t eq,
                                              sys_event_type_t et) {
    // emu doesn't expect exceptions
    //assert(et == SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
    return CELL_OK;
}

int32_t sys_spu_thread_group_disconnect_event_all_threads(sys_spu_thread_group_t id, uint8_t spup) {
    return CELL_OK;
}

#define CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER 0x1
#define CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP 0x2
#define CELL_SPURS_TRACE_MODE_FLAG_MASK 0x3

int32_t cellSpursInitializeWithAttribute1or2(uint32_t spurs_va, uint32_t attr_va, uint32_t fnid) {
    uint32_t index;
    auto entry = findNCallEntry(fnid, index);
    assert(entry); (void)entry;
    auto info = g_state.proc->getStolenInfo(index);
    auto ncall = g_state.mm->load32(info.va);
    g_state.mm->store32(info.va, info.bytes, g_state.granule);

    auto modules = g_state.proc->loadedModules();
    auto initSpursStubVa =
        findExportedSymbol(modules,
                           fnid,
                           "cellSpurs",
                           prx_symbol_type_t::function);
    auto initTraceStubVa = findExportedSymbol(modules,
                                              calcFnid("cellSpursTraceInitialize"),
                                              "cellSpurs",
                                              prx_symbol_type_t::function);
    auto startTraceStubVa = findExportedSymbol(modules,
                                              calcFnid("cellSpursTraceStart"),
                                              "cellSpurs",
                                              prx_symbol_type_t::function);

    g_state.th->ps3call(g_state.mm->load32(initSpursStubVa), [=] {
        g_state.mm->store32(info.va, ncall, g_state.granule);
        if (!spursTrace.enabled)
            return g_state.th->getGPR(3);
        spursTrace.buffer = (char*)g_state.memalloc->allocInternalMemory(
            &spursTrace.bufferVa, spursTraceBufferSize, 128);
        g_state.th->setGPR(3, spurs_va);
        g_state.th->setGPR(4, spursTrace.bufferVa);
        g_state.th->setGPR(5, spursTraceBufferSize);
        g_state.th->setGPR(6, CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER |
                              CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP);
        g_state.th->ps3call(g_state.mm->load32(initTraceStubVa), [&] {
            assert(g_state.th->getGPR(3) == 0);
            g_state.th->setGPR(3, spurs_va);
            g_state.th->ps3call(g_state.mm->load32(startTraceStubVa), [&] {
                assert(g_state.th->getGPR(3) == 0);
                return g_state.th->getGPR(3);
            });
            return g_state.th->getGPR(3);
        });
        return g_state.th->getGPR(3);
    });
    return g_state.th->getGPR(3);
}

std::vector<std::shared_ptr<ThreadGroup>> getThreadGroups() {
    std::vector<std::shared_ptr<ThreadGroup>> res;
    for (auto& g : groups.map()) {
        res.push_back(g.second);
    }
    return res;
}

int32_t cellSpursInitializeWithAttribute(uint32_t spurs_va, uint32_t attr_va) {
    return cellSpursInitializeWithAttribute1or2(
        spurs_va, attr_va, calcFnid("cellSpursInitializeWithAttribute"));
}

int32_t cellSpursInitializeWithAttribute2(uint32_t spurs_va, uint32_t attr_va) {
    return cellSpursInitializeWithAttribute1or2(
        spurs_va, attr_va, calcFnid("cellSpursInitializeWithAttribute2"));
}

int32_t cellSpursFinalize(uint32_t spurs_va) {
    return 0;
//     uint32_t index;
//     auto entry = findNCallEntry(calcFnid("cellSpursFinalize"), index);
//     assert(entry);
//     auto info = g_state.proc->getStolenInfo(index);
//     auto ncall = g_state.mm->load32(info.va);
//     g_state.mm->store32(info.va, info.bytes);
// 
//     auto modules = g_state.proc->loadedModules();
//     auto spursFinalizeStubVa =
//         findExportedSymbol(modules,
//                            calcFnid("cellSpursFinalize"),
//                            "cellSpurs",
//                            prx_symbol_type_t::function);
//     auto stopTraceStubVa = findExportedSymbol(modules,
//                                               calcFnid("cellSpursTraceStop"),
//                                               "cellSpurs",
//                                               prx_symbol_type_t::function);
// 
//     if (!spursTrace.enabled) {
//         g_state.th->ps3call(g_state.mm->load32(spursFinalizeStubVa), [=] {
//             g_state.mm->store32(info.va, ncall);
//             return g_state.th->getGPR(3);
//         });
//     } else {
//         g_state.th->ps3call(g_state.mm->load32(stopTraceStubVa), [=] {
//             assert(g_state.th->getGPR(3) == 0);
//             g_state.mm->store32(info.va, ncall);
//             g_state.th->setGPR(3, spurs_va);
//             g_state.th->ps3call(g_state.mm->load32(spursFinalizeStubVa), [&] {
//                 return g_state.th->getGPR(3);
//             });
//             return g_state.th->getGPR(3);
//         });
//     }
// 
//     return g_state.th->getGPR(3);
}

struct CellSpursTraceInfo {
    big_uint32_t spu_thread[8];
    big_uint32_t count[8];
    big_uint32_t spu_thread_grp;
    big_uint32_t nspu;
    uint8_t padding[128 - sizeof(uint32_t) * (8 + 8 + 2)];
};
static_assert(sizeof(CellSpursTraceInfo) == 128, "");

typedef struct CellSpursTraceHeader {
    uint8_t tag;
    uint8_t length;
    uint8_t spu;
    uint8_t workload;
    big_uint32_t time;
} CellSpursTraceHeader;

#define CELL_SPURS_TRACE_TAG_KERNEL 0x20
#define CELL_SPURS_TRACE_TAG_SERVICE 0x21
#define CELL_SPURS_TRACE_TAG_TASK 0x22
#define CELL_SPURS_TRACE_TAG_JOB 0x23
#define CELL_SPURS_TRACE_TAG_OVIS 0x24
#define CELL_SPURS_TRACE_TAG_LOAD 0x2a
#define CELL_SPURS_TRACE_TAG_MAP 0x2b
#define CELL_SPURS_TRACE_TAG_START 0x2c
#define CELL_SPURS_TRACE_TAG_STOP 0x2d
#define CELL_SPURS_TRACE_TAG_USER 0x2e
#define CELL_SPURS_TRACE_TAG_GUID 0x2f
#define CELL_TRACE_TAG_LOAD 0x50
#define CELL_TRACE_TAG_MAP 0x51
#define CELL_TRACE_TAG_DISPATCH 0x52
#define CELL_TRACE_TAG_RESUME 0x53
#define CELL_TRACE_TAG_EXIT 0x54
#define CELL_TRACE_TAG_YIELD 0x55
#define CELL_TRACE_TAG_SLEEP 0x56
#define CELL_TRACE_TAG_USER 0x57
#define CELL_TRACE_TAG_GUID 0x58


#define CELL_SPURS_TRACE_TAG_CONTROL 0xf0

typedef struct CellSpursTraceControlData {
    big_uint32_t incident;
    big_uint32_t idSpuThread;
} CellSpursTraceControlData;

#define CELL_SPURS_TRACE_CONTROL_START 0x01
#define CELL_SPURS_TRACE_CONTROL_STOP 0x02

typedef struct CellSpursTraceServiceData {
    big_uint32_t incident;
    big_uint32_t __reserved__;
} CellSpursTraceServiceData;

#define CELL_SPURS_TRACE_SERVICE_INIT 0x01
#define CELL_SPURS_TRACE_SERVICE_WAIT 0x02
#define CELL_SPURS_TRACE_SERVICE_EXIT 0x03

typedef struct CellSpursTraceTaskData {
    big_uint32_t incident;
    big_uint32_t task;
} CellSpursTraceTaskData;

#define CELL_SPURS_TRACE_TASK_DISPATCH 0x01
#define CELL_SPURS_TRACE_TASK_YIELD 0x03
#define CELL_SPURS_TRACE_TASK_WAIT 0x04
#define CELL_SPURS_TRACE_TASK_EXIT 0x05


typedef struct CellSpursTraceJobData {
    uint8_t __reserved__[3];
    uint8_t binLSAhigh8;
    big_uint32_t jobDescriptor;
} CellSpursTraceJobData;

typedef struct CellSpursTraceLoadData {
    big_uint32_t ea;
    big_uint16_t ls;
    big_uint16_t size;
} CellSpursTraceLoadData;

typedef struct CellSpursTraceMapData {
    big_uint32_t offset;
    big_uint16_t ls;
    big_uint16_t size;
} CellSpursTraceMapData;

typedef struct CellSpursTraceStartData {
    char module[4];
    big_uint16_t level;
    uint16_t ls;
} CellSpursTraceStartData;

struct CellTraceDispatchData {
    char name[4];
    big_uint32_t va;
};

typedef struct CellSpursTracePacket {
    CellSpursTraceHeader header;
    union {
        CellSpursTraceControlData control;
        CellSpursTraceServiceData service;
        CellSpursTraceTaskData task;
        CellSpursTraceJobData job;
        CellSpursTraceLoadData load;
        CellSpursTraceMapData map;
        CellSpursTraceStartData start;
        CellTraceDispatchData dispatch;
        CellTraceDispatchData resume;
        big_uint64_t stop;
        big_uint64_t user;
        big_uint64_t guid;
        big_uint64_t rawData;
    } data;
} CellSpursTracePacket;

#define CELL_SPURS_TRACE_PACKET_SIZE 16
#define CELL_SPURS_TRACE_BUFFER_ALIGN 16

#define CELL_SPURS_TRACE_MODE_FLAG_WRAP_BUFFER 0x1
#define CELL_SPURS_TRACE_MODE_FLAG_SYNCHRONOUS_START_STOP 0x2
#define CELL_SPURS_TRACE_MODE_FLAG_MASK 0x3

void dumpSpursTrace(std::function<void(std::string)> logLine, char* buffer, uint32_t buffer_size) {
    if (!buffer) buffer = spursTrace.buffer;
    if (!buffer_size) buffer_size = spursTraceBufferSize;
    
    if (!buffer) {
        logLine("No trace yet");
        return;
    }
    
    auto info = (CellSpursTraceInfo*)buffer;
    if (!info->nspu) {
        logLine("No spus initialized yet");
        return;
    }
    logLine(ssnprintf("Tracing SPURS with %d SPUs, buffer va: %08x",
                  info->nspu,
                  spursTrace.bufferVa));
    auto traceAreaSize = ((buffer_size - sizeof(CellSpursTraceInfo)) / info->nspu) & 0xfff0;
    for (auto i = 0u; i < info->nspu; ++i) {
        logLine(ssnprintf("SPU %d trace", i));
        auto packet = (CellSpursTracePacket*)(buffer +
                                              sizeof(CellSpursTraceInfo) + i * traceAreaSize);
        for (;;) {
            if (packet->header.tag == 0)
                break;
            auto log = [&](auto&& msg) {
                logLine(ssnprintf("[SPU%d W:%02x T:%08x] %s",
                                  packet->header.spu,
                                  packet->header.workload,
                                  packet->header.time,
                                  msg));
            };
            switch (packet->header.tag) {
                case CELL_SPURS_TRACE_TAG_SERVICE: {
                    switch (packet->data.service.incident) {
                        case CELL_SPURS_TRACE_SERVICE_INIT:
                            log("SERVICE_INIT");
                            break;
                        case CELL_SPURS_TRACE_SERVICE_WAIT:
                            log("SERVICE_WAIT");
                            break;
                        case CELL_SPURS_TRACE_SERVICE_EXIT:
                            log("SERVICE_EXIT");
                            break;
                        default: assert(false);
                    }
                    break;
                }
                case CELL_SPURS_TRACE_TAG_TASK: {
                    switch (packet->data.task.incident) {
                        case CELL_SPURS_TRACE_TASK_DISPATCH:
                            log(ssnprintf("TASK_DISPATCH: %04x", packet->data.task.task));
                            break;
                        case CELL_SPURS_TRACE_TASK_WAIT:
                            log(ssnprintf("TASK_WAIT: %04x", packet->data.task.task));
                            break;
                        case CELL_SPURS_TRACE_TASK_EXIT:
                            log(ssnprintf("TASK_EXIT: %04x", packet->data.task.task));
                            break;
                        case CELL_SPURS_TRACE_TASK_YIELD:
                            log(ssnprintf("TASK_YIELD: %04x", packet->data.task.task));
                            break;
                        default: assert(false);
                    }
                    break;
                }
                case CELL_SPURS_TRACE_TAG_GUID:
                    log(ssnprintf("TASK_YIELD: %016llx", packet->data.guid));
                    break;
                case CELL_SPURS_TRACE_TAG_LOAD:
                    log(ssnprintf("USER_LOAD: ea=%08u, ls=%06x, size=%x",
                                  packet->data.load.ea,
                                  packet->data.load.ls * 16,
                                  packet->data.load.size));
                    break;
                case CELL_SPURS_TRACE_TAG_MAP:
                    log(ssnprintf("USER_MAP: offset=%08u, ls=%06x, size=%x",
                                  packet->data.map.offset,
                                  packet->data.map.ls * 16,
                                  packet->data.map.size));
                    break;
                case CELL_SPURS_TRACE_TAG_START: {
                    auto level = packet->data.start.level == 0 ? "kernel"
                               : packet->data.start.level == 1 ? "policy" 
                               : "job/task";
                    log(ssnprintf("USER_START: name=%s, level=%04x(%s), ls=%x",
                                  std::string(packet->data.start.module, 4),
                                  packet->data.start.level,
                                  level,
                                  packet->data.start.ls * 16));
                    break;
                }
                case CELL_SPURS_TRACE_TAG_STOP:
                    log(ssnprintf("USER_STOP: %016llx", packet->data.stop));
                    break;
                case CELL_SPURS_TRACE_TAG_KERNEL:
                    log(ssnprintf("KERNEL: %016llx", packet->data.guid));
                    break;
                case CELL_TRACE_TAG_LOAD:
                    log(ssnprintf("LOAD: ea=%08u, ls=%x, size=%04x",
                        packet->data.load.ea,
                        packet->data.load.ls * 16,
                        packet->data.load.size));
                    break;
                case CELL_TRACE_TAG_MAP:
                    log(ssnprintf("MAP: offset=%08u, ls=%x, size=%04x",
                                  packet->data.map.offset,
                                  packet->data.map.ls * 16,
                                  packet->data.map.size));
                    break;
                case CELL_TRACE_TAG_DISPATCH:
                    log(ssnprintf("DISPATCH: name=%s, va=%08x",
                                  std::string(packet->data.dispatch.name, 4),
                                  packet->data.dispatch.va));
                    break;
                case CELL_TRACE_TAG_RESUME:
                    log(ssnprintf("RESUME: name=%s, va=%08x",
                                  std::string(packet->data.dispatch.name, 4),
                                  packet->data.dispatch.va));
                    break;
                case CELL_TRACE_TAG_EXIT:
                    log(ssnprintf("EXIT: %016llx", packet->data.guid));
                    break;
                case CELL_TRACE_TAG_YIELD:
                    log(ssnprintf("YIELD: %016llx", packet->data.guid));
                    break;
                case CELL_TRACE_TAG_SLEEP:
                    log(ssnprintf("SLEEP: %016llx", packet->data.guid));
                    break;
                case CELL_TRACE_TAG_GUID:
                    log(ssnprintf("GUID: %016llx", packet->data.guid));
                    break;
                case CELL_TRACE_TAG_USER:
                    log(ssnprintf("USER: %016llx", packet->data.guid));
                    break;
                default:
                    log(ssnprintf("UNKNOWN TAG: %02x, %016llx", packet->header.tag, packet->data.guid));
                    break;
            }
            packet++;
        }
    }
}

void enableSpursTrace() {
    spursTrace.enabled = true;
}
