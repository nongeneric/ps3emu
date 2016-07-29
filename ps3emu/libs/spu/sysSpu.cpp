#include "sysSpu.h"

#include "SpuImage.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/ppu/InterruptPPUThread.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/utils.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/log.h"
#include "ps3emu/spu/SPUChannels.h"
#include <boost/range/algorithm.hpp>
#include <array>
#include <vector>
#include <fstream>
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
    SPUThread* thread;
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
    
#if DEBUG
    std::ofstream f("/tmp/ps3emu_lastSpuImage.elf");
    assert(f.is_open());
    std::vector<char> bytes(1 << 20);
    th->mm()->readMemory(src, &bytes[0], bytes.size(), true);
    f.write(&bytes[0], bytes.size());
    f.close();
#endif
    
    img->elf = new SpuImage([=](uint32_t ptr, void* buf, size_t size) {
        th->mm()->readMemory(ptr, buf, size);
    }, src);
    return CELL_OK;
}

int32_t sys_spu_image_open(sys_spu_image_t* img, cstring_ptr_t path, Process* proc) {
    LOG << ssnprintf("sys_spu_image_open(\"%s\")", path.str);
    auto hostPath = proc->contentManager()->toHost(path.str.c_str());
    auto elfPath = hostPath;
    if (hostPath.substr(hostPath.size() - 4) != ".elf") {
        elfPath += ".elf";
    }
    std::ifstream f(elfPath);
    if (!f.is_open()) {
        auto message = ssnprintf("sys_spu_image_open: elf not found (\"%s\")", elfPath);
        LOG << message;
        throw std::runtime_error(message);
    }
    img->elf = new SpuImage([&](uint32_t ptr, void* buf, size_t size) {
        f.seekg(ptr);
        f.read((char*)buf, size);
    }, 0);
    return CELL_OK;
}

int32_t sys_spu_image_close(sys_spu_image_t* img) {
    LOG << __FUNCTION__;
    delete img->elf;
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

void initThread(SPUThread* thread, SpuImage* elf) {
    memcpy(thread->ptr(0), elf->localStorage(), LocalStorageSize);
    thread->setNip(elf->entryPoint());
    thread->setElfSource(elf->source());
}

int32_t sys_spu_thread_initialize(sys_spu_thread_t* thread_id,
                                  sys_spu_thread_group_t group_id,
                                  uint32_t spu_num,
                                  sys_spu_image_t* img,
                                  const sys_spu_thread_attribute_t* attr,
                                  const sys_spu_thread_argument_t* arg,
                                  Process* proc) {
    LOG << ssnprintf("sys_spu_thread_initialize() source=%x", img->elf->source());
    auto group = groups.get(group_id);
    std::string name;
    name.resize(attr->nsize);
    proc->mm()->readMemory(attr->name, &name[0], attr->nsize);
    
    *thread_id = proc->createSpuThread(name);
    auto thread = proc->getSpuThread(*thread_id);
    thread->setSpu(spu_num);
    initThread(thread, img->elf);
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
        proc->destroySpuThread(th);
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

int32_t sys_raw_spu_create(sys_raw_spu_t* id, uint32_t unused, Process* proc) {
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
    if (rawSpu->thread->tryJoin().cause == SPUThreadExitCause::StillRunning) {
        // leave a hanging thread if couldn't join
        assert(hangingRawSpuThreads < 10);
        proc->destroySpuThread(rawSpu->thread);
        rawSpus.destroy(id);
        hangingRawSpuThreads++;
    }
    return CELL_OK;
}

int32_t sys_raw_spu_image_load(sys_raw_spu_t id, sys_spu_image_t* img) {
    auto rawSpu = rawSpus.get(id);
    initThread(rawSpu->thread, img->elf);
    return CELL_OK;
}

int32_t sys_raw_spu_load(sys_raw_spu_t id, cstring_ptr_t path, big_uint32_t* entry, Process* proc) {
    auto rawSpu = rawSpus.get(id);
    sys_spu_image_t img;
    sys_spu_image_open(&img, path, proc);
    *entry = img.elf->entryPoint();
    initThread(rawSpu->thread, img.elf);
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
    th->establish(tag->rawSpu->thread);
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
    *stat = rawSpu->thread->channels()->mmio_read(Prxy_QueryType);
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
    rawSpu->thread->getStatus() = stat;
    return CELL_OK;
}

emu_void_t sys_interrupt_thread_eoi() {
    throw ThreadFinishedException(0);
}

SPUThread* findRawSpuThread(sys_raw_spu_t id) {
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
