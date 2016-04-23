#include "sysSpu.h"

#include "../ps3emu/ELFLoader.h"
#include "../ps3emu/IDMap.h"
#include "../ps3emu/ppu/InterruptPPUThread.h"
#include "../ps3emu/spu/SPUThread.h"
#include "../ps3emu/utils.h"
#include "../ps3emu/ContentManager.h"
#include <boost/log/trivial.hpp>
#include <array>
#include <vector>
#include <fstream>
#include <assert.h>

struct Elf32_be_Ehdr {
    boost::endian::big_uint8_t e_ident[EI_NIDENT]; /* Magic number and other info */
    boost::endian::big_uint16_t e_type;            /* Object file type */
    boost::endian::big_uint16_t e_machine;         /* Architecture */
    boost::endian::big_uint32_t e_version;         /* Object file version */
    boost::endian::big_uint32_t e_entry;           /* Entry point virtual address */
    boost::endian::big_uint32_t e_phoff;     /* Program header table file offset */
    boost::endian::big_uint32_t e_shoff;     /* Section header table file offset */
    boost::endian::big_uint32_t e_flags;     /* Processor-specific flags */
    boost::endian::big_uint16_t e_ehsize;    /* ELF header size in bytes */
    boost::endian::big_uint16_t e_phentsize; /* Program header table entry size */
    boost::endian::big_uint16_t e_phnum;     /* Program header table entry count */
    boost::endian::big_uint16_t e_shentsize; /* Section header table entry size */
    boost::endian::big_uint16_t e_shnum;     /* Section header table entry count */
    boost::endian::big_uint16_t e_shstrndx;  /* Section header string table index */
};

struct Elf32_be_Phdr {
    boost::endian::big_uint32_t p_type;   /* Segment type */
    boost::endian::big_uint32_t p_offset; /* Segment file offset */
    boost::endian::big_uint32_t p_vaddr;  /* Segment virtual address */
    boost::endian::big_uint32_t p_paddr;  /* Segment physical address */
    boost::endian::big_uint32_t p_filesz; /* Segment size in file */
    boost::endian::big_uint32_t p_memsz;  /* Segment size in memory */
    boost::endian::big_uint32_t p_flags;  /* Segment flags */
    boost::endian::big_uint32_t p_align;  /* Segment alignment */
};

struct Elf32_be_Shdr {
    boost::endian::big_uint32_t sh_name;      /* Section name (string tbl index) */
    boost::endian::big_uint32_t sh_type;      /* Section type */
    boost::endian::big_uint32_t sh_flags;     /* Section flags */
    boost::endian::big_uint32_t sh_addr;      /* Section virtual addr at execution */
    boost::endian::big_uint32_t sh_offset;    /* Section file offset */
    boost::endian::big_uint32_t sh_size;      /* Section size in bytes */
    boost::endian::big_uint32_t sh_link;      /* Link to another section */
    boost::endian::big_uint32_t sh_info;      /* Additional section information */
    boost::endian::big_uint32_t sh_addralign; /* Section alignment */
    boost::endian::big_uint32_t sh_entsize;   /* Entry size if section holds table */
};

struct Elf32_be_Sym {
    boost::endian::big_uint32_t st_name;  /* Symbol name (string tbl index) */
    boost::endian::big_uint8_t st_info;   /* Symbol type and binding */
    boost::endian::big_uint8_t st_other;  /* Symbol visibility */
    boost::endian::big_uint16_t st_shndx; /* Section index */
    boost::endian::big_uint32_t st_value; /* Symbol value */
    boost::endian::big_uint32_t st_size;  /* Symbol size */
};

static_assert(sizeof(Elf32_be_Ehdr) == sizeof(Elf32_Ehdr),
              "big endian struct mismatch");
static_assert(sizeof(Elf32_be_Phdr) == sizeof(Elf32_Phdr),
              "big endian struct mismatch");
static_assert(sizeof(Elf32_be_Shdr) == sizeof(Elf32_Shdr),
              "big endian struct mismatch");

#define SYS_SPU_THREAD_GROUP_TYPE_NORMAL                 0x00
#define SYS_SPU_THREAD_GROUP_TYPE_SEQUENTIAL             0x01
#define SYS_SPU_THREAD_GROUP_TYPE_SYSTEM                 0x02
#define SYS_SPU_THREAD_GROUP_TYPE_MEMORY_FROM_CONTAINER  0x04
#define SYS_SPU_THREAD_GROUP_TYPE_NON_CONTEXT            0x08
#define SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT  0x18
#define SYS_SPU_THREAD_GROUP_TYPE_COOPERATE_WITH_SYSTEM  0x20

class SpuImage {
    std::array<uint8_t, LocalStorageSize> _ls;
    std::string _desc;
    uint32_t _ep;
    uint32_t _src;

public:
    SpuImage(std::function<void(uint32_t, void*, size_t)> read, ps3_uintptr_t src)
        : _src(src) {
        Elf32_be_Ehdr header;
        read(src, &header, sizeof(header));

        _ep = header.e_entry;

        assert(sizeof(Elf32_Phdr) == header.e_phentsize);
        std::vector<Elf32_be_Phdr> phs(header.e_phnum);
        read(src + header.e_phoff, &phs[0], phs.size() * sizeof(Elf32_Phdr));

        memset(&_ls[0], 0, _ls.size());
        for(auto& ph : phs) {
            assert(ph.p_type != PT_LOAD || ph.p_type != PT_NOTE);
            if(ph.p_type == PT_NOTE) {
                std::vector<char> vec(ph.p_filesz);
                read(src + ph.p_offset, &vec[0], ph.p_filesz);
                _desc = std::string(&vec[20]);
                continue;
            }
            if(ph.p_vaddr % ph.p_align != 0)
                throw std::runtime_error("complex alignment not supported");
            if(ph.p_memsz == 0)
                continue;

            BOOST_LOG_TRIVIAL(trace) << ssnprintf("mapping segment of size %" PRIx64
                                                  " to %" PRIx64 "-%" PRIx64,
                                                  (uint64_t)ph.p_filesz,
                                                  (uint64_t)ph.p_vaddr,
                                                  (ph.p_paddr + ph.p_memsz));

            assert(ph.p_memsz >= ph.p_filesz);
            read(src + ph.p_offset, &_ls[ph.p_vaddr], ph.p_filesz);
        }
    }
    
    uint8_t* localStorage() {
        return &_ls[0];
    }
    
    uint32_t entryPoint() {
        return _ep;
    }
    
    uint32_t source() {
        return _src;
    }
};

struct ThreadGroup {
    std::vector<uint32_t> threads;
    std::string name;
    std::map<uint32_t, int32_t> errorCodes;
};

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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_spu_initialize(uint32_t max_usable_spu, uint32_t max_raw_spu) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t sys_spu_image_import(sys_spu_image_t* img,
                             ps3_uintptr_t src,
                             uint32_t type,
                             MainMemory* mm) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    img->elf = new SpuImage([=](uint32_t ptr, void* buf, size_t size) {
        mm->readMemory(ptr, buf, size);
    }, src);
    return CELL_OK;
}

int32_t sys_spu_image_open(sys_spu_image_t* img, cstring_ptr_t path, Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_spu_image_open(\"%s\")", path.str);
    auto hostPath = proc->contentManager()->toHost(path.str.c_str());
    auto elfPath = hostPath;
    if (hostPath.substr(hostPath.size() - 4) != ".elf") {
        elfPath += ".elf";
    }
    std::ifstream f(elfPath);
    if (!f.is_open()) {
        auto message = ssnprintf("sys_spu_image_open: elf not found (\"%s\")", elfPath);
        BOOST_LOG_TRIVIAL(fatal) << message;
        throw std::runtime_error(message);
    }
    img->elf = new SpuImage([&](uint32_t ptr, void* buf, size_t size) {
        f.seekg(ptr);
        f.read((char*)buf, size);
    }, 0);
    return CELL_OK;
}

int32_t sys_spu_image_close(sys_spu_image_t* img) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    delete img->elf;
    return CELL_OK;
}

int32_t sys_spu_thread_group_create(sys_spu_thread_group_t* id,
                                    uint32_t num,
                                    int32_t prio,
                                    sys_spu_thread_group_attribute_t* attr,
                                    MainMemory* mm) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(attr->type == SYS_SPU_THREAD_GROUP_TYPE_NORMAL);
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
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("sys_spu_thread_initialize() source=%x", img->elf->source());
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
    if (rawSpu->thread->join().cause == SPUThreadExitCause::StillRunning) {
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
                                         TagClassId class_id,
                                         uint32_t unused,
                                         sys_interrupt_tag_t* intrtag) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    auto tag = std::make_shared<InterruptTag>();
    tag->classId = class_id;
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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(class_id == 2);
    auto rawSpu = rawSpus.get(id);
    auto interruptThread = rawSpu->tag->interruptThread;
    interruptThread->setMask2(mask);
    return CELL_OK;
}

#undef X
#define X(k, v) case TagClassId::k: return #k;
const char* tagToString(TagClassId tag) {
    switch (tag) { TagClassIdX }
    return "";
}

int32_t sys_raw_spu_mmio_write(sys_raw_spu_t id,
                               TagClassId classId,
                               uint32_t value,
                               Process* proc) {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf(
        "ppu writes %x via mmio to spu %d tag %s", value, id, tagToString(classId));
    auto rawSpu = rawSpus.get(id);
    if (classId == TagClassId::SPU_RunCntl && value == 1) {
        rawSpu->thread->run();
        return CELL_OK;
    }
    if (classId == TagClassId::SPU_In_MBox) {
        rawSpu->thread->getToSpuMailbox().send(value);
        return CELL_OK;
    }
    if (classId == TagClassId::_MFC_LSA) {
        rawSpu->thread->ch(MFC_LSA) = value;
        return CELL_OK;
    }
    if (classId == TagClassId::_MFC_EAH) {
        rawSpu->thread->ch(MFC_EAH) = value;
        return CELL_OK;
    }
    if (classId == TagClassId::_MFC_EAL) {
        rawSpu->thread->ch(MFC_EAL) = value;
        return CELL_OK;
    }
    if (classId == TagClassId::MFC_Size_Tag) {
        rawSpu->thread->ch(MFC_Size) = value >> 16;
        rawSpu->thread->ch(MFC_TagID) = value & 0xff;
        return CELL_OK;
    }
    if (classId == TagClassId::MFC_Class_CMD) {
        rawSpu->thread->command(value);
        return CELL_OK;
    }
    if (classId == TagClassId::Prxy_QueryMask) {
        rawSpu->thread->ch(MFC_WrTagMask) = value;
        return CELL_OK;
    }
    if (classId == TagClassId::Prxy_QueryType) {
        // do nothing, every request is completed immediately and
        // as such there is no difference between any (01) and all (10) tag groups
        // also disabling completion (00) notifications makes no sense
        return CELL_OK;
    }
    if (classId == TagClassId::SPU_NPC) {
        rawSpu->thread->setNip(value);
        return CELL_OK;
    }
    throw std::runtime_error("unknown mmio offset");
}

uint32_t sys_raw_spu_mmio_read_impl(sys_raw_spu_t id, TagClassId classId, Process* proc) {
    auto rawSpu = rawSpus.get(id);
    switch (classId) {
        case TagClassId::SPU_Status: return rawSpu->thread->getStatus();
        case TagClassId::SPU_Out_MBox:
            return rawSpu->thread->getFromSpuMailbox().receive(0);
        case TagClassId::MFC_Class_CMD: return rawSpu->thread->ch(MFC_Cmd);
        case TagClassId::MFC_QStatus:
            return 0x8000ffff; // all mfc requests complete immediately
        // mailbox queues are implemented as blocking so there is no need for PPU to
        // busy-wait on them
        // 0x010101 means OutIntrMbox and OutMbox contain a single item; InMbox has
        // at least one empty slot
        case TagClassId::SPU_MBox_Status: return 0x010101;
        default: throw std::runtime_error("unknown mmio offset");
    }
}

uint32_t sys_raw_spu_mmio_read(sys_raw_spu_t id, TagClassId classId, Process* proc) {
    uint32_t value = sys_raw_spu_mmio_read_impl(id, classId, proc);
    BOOST_LOG_TRIVIAL(trace) << ssnprintf(
        "ppu reads %x via mmio from spu %d tag %s", value, id, tagToString(classId));
    return value;
}

int32_t sys_raw_spu_get_int_stat(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 big_uint64_t* stat) {
    if (class_id != 2) {
        throw std::runtime_error("incorrect spu interrupt class");
    }
    auto rawSpu = rawSpus.get(id);
    *stat = rawSpu->thread->getStatus();
    return CELL_OK;
}

int32_t sys_raw_spu_read_puint_mb(sys_raw_spu_t id, big_uint32_t* value) {
    auto rawSpu = rawSpus.get(id);
    *value = rawSpu->thread->getFromSpuInterruptMailbox().receive(0);
    return CELL_OK;
}

int32_t sys_raw_spu_set_int_stat(sys_raw_spu_t id, uint32_t class_id, uint64_t stat) {
    auto rawSpu = rawSpus.get(id);
    rawSpu->thread->getStatus() = stat;
    return CELL_OK;
}

emu_void_t sys_interrupt_thread_eoi() {
    throw ThreadFinishedException(0);
}

SPUThread* findRawSpuThread(sys_raw_spu_t id) {
    return rawSpus.get(id)->thread;
}