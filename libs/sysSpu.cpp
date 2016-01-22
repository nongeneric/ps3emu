#include "sysSpu.h"

#include "../ps3emu/ELFLoader.h"
#include "../ps3emu/IDMap.h"
#include "../ps3emu/spu/SPUThread.h"
#include "../ps3emu/utils.h"
#include <boost/log/trivial.hpp>
#include <array>
#include <vector>
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
    SpuImage(MainMemory* mm, ps3_uintptr_t src) : _src(src) {
        Elf32_be_Ehdr header;
        mm->readMemory(src, &header, sizeof(header));

        _ep = header.e_entry;

        assert(sizeof(Elf32_Phdr) == header.e_phentsize);
        std::vector<Elf32_be_Phdr> phs(header.e_phnum);
        mm->readMemory(
            src + header.e_phoff, &phs[0], phs.size() * sizeof(Elf32_Phdr));

        memset(&_ls[0], 0, _ls.size());
        for(auto& ph : phs) {
            assert(ph.p_type != PT_LOAD || ph.p_type != PT_NOTE);
            if(ph.p_type == PT_NOTE) {
                std::vector<char> vec(ph.p_filesz);
                mm->readMemory(src + ph.p_offset, &vec[0], ph.p_filesz);
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
            mm->readMemory(src + ph.p_offset, &_ls[ph.p_vaddr], ph.p_filesz);
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

namespace {
    ThreadSafeIDMap<sys_spu_thread_group_t, ThreadGroup> groups;
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
    img->elf = new SpuImage(mm, src);
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
    memcpy(thread->ptr(0), img->elf->localStorage(), LocalStorageSize);
    thread->r(3).dw<0>() = arg->arg1;
    thread->r(4).dw<0>() = arg->arg2;
    thread->r(5).dw<0>() = arg->arg3;
    thread->r(6).dw<0>() = arg->arg4;
    thread->setNip(img->elf->entryPoint());
    thread->setElfSource(img->elf->source());
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