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

#define SYS_SPU_THREAD_GROUP_TYPE_NORMAL                 0x00
#define SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL             0x01
#define SYS_SPU_THREAD_GROUP_TYPE_SYSTEM                 0x02
#define SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER  0x04
#define SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT            0x08
#define SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT  0x18
#define SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM  0x20

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
}

#define SYS_SPU_IMAGE_TYPE_USER     0x0U
#define SYS_SPU_IMAGE_TYPE_KERNEL   0x1U
#define SYS_SPU_SEGMENT_TYPE_COPY  0x0001
#define SYS_SPU_SEGMENT_TYPE_FILL  0x0002
#define SYS_SPU_SEGMENT_TYPE_INFO  0x0004
#define SYS_SPU_IMAGE_PROTECT       0x0U
#define SYS_SPU_IMAGE_DIRECT        0x1U

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
            auto namesz = mm->load<4>(pos);
            pos += 4;
            auto descsz = mm->load<4>(pos);
            pos += 4;
            auto type = mm->load<4>(pos);
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

void spuImageMap(MainMemory* mm, sys_spu_image_t* image, void* ls) {
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
                               big_uint64_t* value,
                               size_t type) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_spu_initialize(uint32_t max_usable_spu, uint32_t max_raw_spu) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_spu_image_import(sys_spu_image_t* img,
                             ps3_uintptr_t src,
                             uint32_t type,
                             PPUThread* th) {
    LOG << __FUNCTION__;
    
    assert(type == SYS_SPU_IMAGE_DIRECT);
    
#if DEBUG
    FILE* f = fopen("/tmp/ps3emu_lastSpuImage.elf", "w");
    assert(f);
    std::vector<char> bytes(1 << 20);
    g_state.mm->readMemory(src, &bytes[0], bytes.size(), true);
    fwrite(&bytes[0], 1, bytes.size(), f);
    fclose(f);
#endif
    
    spuImageInit(g_state.mm, g_state.memalloc, img, src, true);
    return CELL_OK;
}

int32_t sys_spu_image_open(sys_spu_image_t* img, cstring_ptr_t path, Process* proc) {
    LOG << ssnprintf("sys_spu_image_open(\"%s\")", path.str);
    auto hostPath = g_state.content->toHost(path.str.c_str());
    auto elfPath = hostPath;
    if (hostPath.substr(hostPath.size() - 4) != ".elf") {
        elfPath += ".elf";
    }
    FILE* f = fopen(elfPath.c_str(), "r");
    if (!f) {
        auto message = ssnprintf("sys_spu_image_open: elf not found (\"%s\")", elfPath);
        LOG << message;
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

int32_t sys_spu_thread_group_create(sys_spu_thread_group_t* id,
                                    uint32_t num,
                                    int32_t prio,
                                    sys_spu_thread_group_attribute_t* attr,
                                    MainMemory* mm) {
    LOG << __FUNCTION__;
    //assert(attr->type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL);
    auto group = std::make_shared<ThreadGroup>();
    group->name.resize(attr->nsize);
    mm->readMemory(attr->name, &group->name[0], attr->nsize);
    *id = groups.create(group);
    return CELL_OK;
}

void initThread(MainMemory* mm, SPUThread* thread, sys_spu_image_t* image) {
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
    LOG << ssnprintf("sys_spu_thread_initialize() source=%x", img->segs);
    auto group = groups.get(group_id);
    std::string name;
    name.resize(attr->nsize);
    g_state.mm->readMemory(attr->name, &name[0], attr->nsize);
    
    *thread_id = proc->createSpuThread(name);
    auto thread = proc->getSpuThread(*thread_id);
    thread->setSpu(spu_num);
    initThread(g_state.mm, thread.get(), img);
    thread->r(3).dw<0>() = arg->arg1;
    thread->r(4).dw<0>() = arg->arg2;
    thread->r(5).dw<0>() = arg->arg3;
    thread->r(6).dw<0>() = arg->arg4;
    group->threads.push_back(*thread_id);
    return CELL_OK;
}

int32_t sys_spu_thread_group_start(sys_spu_thread_group_t id, Process* proc) {
    auto group = groups.get(id);
    for (auto id : group->threads) {
        auto th = proc->getSpuThread(id);
        th->run();
    }
    return CELL_OK;
}

#define SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT            0x0001
#define SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT      0x0002
#define SYS_SPU_THREAD_GROUP_JOIN_TERMINATED            0x0004

int32_t sys_spu_thread_group_join(sys_spu_thread_group_t gid,
                                  big_int32_t* cause,
                                  big_int32_t* status,
                                  Process* proc)
{
    auto group = groups.get(gid);
    bool groupExit = false;
    bool threadExit = true;
    bool groupTerminate = false;
    int32_t terminateStatus;
    int32_t groupExitStatus;
    for (auto id : group->threads) {
        auto th = proc->getSpuThread(id);
        auto info = th->join();
        groupExit |= info.cause == SPUThreadExitCause::GroupExit;
        groupTerminate |= info.cause == SPUThreadExitCause::GroupTerminate;
        threadExit &= info.cause == SPUThreadExitCause::Exit;
        if (groupTerminate) {
            terminateStatus = info.status;
        }
        if (groupExit) {
            groupExitStatus = info.status;
        }
        group->errorCodes[id] = info.status;
    }
    if (groupTerminate) {
        *cause = SYS_SPU_THREAD_GROUP_JOIN_TERMINATED;
        *status = terminateStatus;
    } else if (groupExit) {
        *cause = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;
        *status = groupExitStatus;
    } else if (threadExit) {
        *cause = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
    }
    return CELL_OK;
}

int32_t sys_spu_thread_group_destroy(sys_spu_thread_group_t id, Process* proc) {
    auto group = groups.get(id);
    for (auto id : group->threads) {
        auto th = proc->getSpuThread(id);
        proc->destroySpuThread(th.get());
    }
    groups.destroy(id);
    return CELL_OK;
}

int32_t sys_spu_thread_get_exit_status(sys_spu_thread_t id,
                                       big_int32_t* status,
                                       Process* proc) {
    for (auto& groupPair : groups.map()) {
        for (auto& threadPair : groupPair.second->errorCodes) {
            if (threadPair.first == id) {
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
        proc->destroySpuThread(rawSpu->thread.get());
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
    sys_spu_image_open(&img, path, proc);
    *entry = img.entry_point;
    initThread(g_state.mm, rawSpu->thread.get(), &img);
    return CELL_OK;
}

int32_t sys_raw_spu_create_interrupt_tag(sys_raw_spu_t id,
                                         unsigned class_id,
                                         uint32_t unused,
                                         sys_interrupt_tag_t* intrtag) {
    LOG << __FUNCTION__;
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
                                       uint64_t arg,
                                       Process* proc) {
    LOG << __FUNCTION__;
    auto th = dynamic_cast<InterruptPPUThread*>(proc->getThread(intrthread));
    auto tag = interruptTags.get(intrtag);
    tag->interruptThread = th;
    th->setArg(arg);
    th->establish(tag->rawSpu->thread.get());
    return CELL_OK;
}

int32_t sys_raw_spu_set_int_mask(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 uint64_t mask) {
    LOG << __FUNCTION__;
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
        throw std::runtime_error("incorrect spu interrupt class");
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

int32_t sys_raw_spu_set_int_stat(sys_raw_spu_t id, uint32_t class_id, uint64_t stat) {
    if (class_id != 2) {
        throw std::runtime_error("incorrect spu interrupt class");
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
    assert(et == SYS_SPU_THREAD_GROUP_EVENT_EXCEPTION);
    return CELL_OK;
}
