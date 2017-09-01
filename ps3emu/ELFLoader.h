#pragma once

#include "MainMemory.h"
#include "ps3emu/rewriter.h"
#include "ps3emu/state.h"
#include <stdint.h>
#include <elf.h>
#include <string>
#include <vector>
#include <functional>
#include <tuple>
#include <memory>

#include <boost/endian/arithmetic.hpp>

typedef struct
{
    boost::endian::big_uint8_t    e_ident[EI_NIDENT];     /* Magic number and other info */
    boost::endian::big_uint16_t   e_type;                 /* Object file type */
    boost::endian::big_uint16_t   e_machine;              /* Architecture */
    boost::endian::big_uint32_t   e_version;              /* Object file version */
    boost::endian::big_uint64_t   e_entry;                /* Entry point virtual address */
    boost::endian::big_uint64_t   e_phoff;                /* Program header table file offset */
    boost::endian::big_uint64_t   e_shoff;                /* Section header table file offset */
    boost::endian::big_uint32_t   e_flags;                /* Processor-specific flags */
    boost::endian::big_uint16_t   e_ehsize;               /* ELF header size in bytes */
    boost::endian::big_uint16_t   e_phentsize;            /* Program header table entry size */
    boost::endian::big_uint16_t   e_phnum;                /* Program header table entry count */
    boost::endian::big_uint16_t   e_shentsize;            /* Section header table entry size */
    boost::endian::big_uint16_t   e_shnum;                /* Section header table entry count */
    boost::endian::big_uint16_t   e_shstrndx;             /* Section header string table index */
} Elf64_be_Ehdr;

typedef struct
{
    boost::endian::big_uint32_t   p_type;                 /* Segment type */
    boost::endian::big_uint32_t   p_flags;                /* Segment flags */
    boost::endian::big_uint64_t   p_offset;               /* Segment file offset */
    boost::endian::big_uint64_t   p_vaddr;                /* Segment virtual address */
    boost::endian::big_uint64_t   p_paddr;                /* Segment physical address */
    boost::endian::big_uint64_t   p_filesz;               /* Segment size in file */
    boost::endian::big_uint64_t   p_memsz;                /* Segment size in memory */
    boost::endian::big_uint64_t   p_align;                /* Segment alignment */
} Elf64_be_Phdr;

typedef struct
{
    boost::endian::big_uint32_t   sh_name;                /* Section name (string tbl index) */
    boost::endian::big_uint32_t   sh_type;                /* Section type */
    boost::endian::big_uint64_t   sh_flags;               /* Section flags */
    boost::endian::big_uint64_t   sh_addr;                /* Section virtual addr at execution */
    boost::endian::big_uint64_t   sh_offset;              /* Section file offset */
    boost::endian::big_uint64_t   sh_size;                /* Section size in bytes */
    boost::endian::big_uint32_t   sh_link;                /* Link to another section */
    boost::endian::big_uint32_t   sh_info;                /* Additional section information */
    boost::endian::big_uint64_t   sh_addralign;           /* Section alignment */
    boost::endian::big_uint64_t   sh_entsize;             /* Entry size if section holds table */
} Elf64_be_Shdr;

typedef struct
{
    boost::endian::big_uint32_t   st_name;                /* Symbol name (string tbl index) */
    boost::endian::big_uint8_t    st_info;                /* Symbol type and binding */
    boost::endian::big_uint8_t    st_other;               /* Symbol visibility */
    boost::endian::big_uint16_t   st_shndx;               /* Section index */
    boost::endian::big_uint64_t   st_value;               /* Symbol value */
    boost::endian::big_uint64_t   st_size;                /* Symbol size */
} Elf64_be_Sym;

typedef struct {
    boost::endian::big_uint64_t	r_offset;
    boost::endian::big_uint64_t	r_info;
    boost::endian::big_uint64_t	r_addend;
} Elf64_be_Rela;

struct fdescr {
    boost::endian::big_uint32_t va;
    boost::endian::big_uint32_t tocBase;
};

struct module_info_t {
    boost::endian::big_uint16_t attributes;
    char minor_version;
    char major_version;
    char name[28];
    boost::endian::big_uint32_t toc;
    boost::endian::big_uint32_t exports_start;
    boost::endian::big_uint32_t exports_end;
    boost::endian::big_uint32_t imports_start;
    boost::endian::big_uint32_t imports_end;
};

struct prx_export_t {
    char size;
    char padding;
    boost::endian::big_uint16_t version;
    boost::endian::big_uint16_t attributes;
    boost::endian::big_uint16_t functions;
    boost::endian::big_uint16_t variables;
    boost::endian::big_uint16_t tls_variables;
    char hash;
    char tls_hash;
    char reserved[2];
    boost::endian::big_uint32_t name;
    boost::endian::big_uint32_t fnid_table;
    boost::endian::big_uint32_t stub_table;
};

struct prx_import_t {
    char size;
    char padding;
    boost::endian::big_uint16_t version;
    boost::endian::big_uint16_t attributes;
    boost::endian::big_uint16_t functions;
    boost::endian::big_uint16_t variables;
    boost::endian::big_uint16_t tls_variables;
    boost::endian::big_uint32_t reserved;
    boost::endian::big_uint32_t name;
    boost::endian::big_uint32_t fnids;
    boost::endian::big_uint32_t fstubs;
    boost::endian::big_uint32_t vnids;
    boost::endian::big_uint32_t vstubs;
    boost::endian::big_uint32_t tls_vnids;
    boost::endian::big_uint32_t tls_vstubs;
};

struct sys_process_prx_info {
    boost::endian::big_uint32_t header_size;
    boost::endian::big_uint32_t magic;
    boost::endian::big_uint32_t version;
    boost::endian::big_uint32_t sdk_version;
    boost::endian::big_uint32_t exports_start;
    boost::endian::big_uint32_t exports_end;
    boost::endian::big_uint32_t imports_start;
    boost::endian::big_uint32_t imports_end;
};

static_assert(sizeof(fdescr) == 8, "");
static_assert(sizeof(Elf64_be_Ehdr) == sizeof(Elf64_Ehdr), "big endian struct mismatch");
static_assert(sizeof(Elf64_be_Phdr) == sizeof(Elf64_Phdr), "big endian struct mismatch");
static_assert(sizeof(Elf64_be_Shdr) == sizeof(Elf64_Shdr), "big endian struct mismatch");
static_assert(sizeof(Elf64_be_Sym) == sizeof(Elf64_Sym), "big endian struct mismatch");
static_assert(sizeof(Elf64_be_Rela) == sizeof(Elf64_Rela), "big endian struct mismatch");
static_assert(sizeof(module_info_t) == 0x34, "");
static_assert(sizeof(prx_export_t) == 0x1c, "");
static_assert(sizeof(prx_import_t) == 0x2c, "");

struct ThreadInitInfo {
    void* tlsBase;
    uint32_t tlsSegmentVa;
    uint32_t tlsFileSize;
    uint32_t tlsMemSize;
    uint32_t primaryStackSize;
    ps3_uintptr_t entryPointDescriptorVa;
};

enum class prx_symbol_type_t {
    function, variable, tls_variable
};

using make_segment_t = std::function<void(ps3_uintptr_t va, uint32_t size, unsigned index)>;

struct StolenFuncInfo {
    uint32_t va;
    uint32_t exportStubVa;
    uint32_t bytes;
    uintptr_t ncallIndex;
};

class RewriterStore {
    std::vector<RewrittenSegment> _ppuModules;
    std::vector<RewrittenSegment> _spuModules;
    
public:
    inline unsigned addPPU(const RewrittenSegment *segment) {
        _ppuModules.push_back(*segment);
        return _ppuModules.size() - 1;
    }
    
    inline unsigned addSPU(const RewrittenSegment *segment) {
        _spuModules.push_back(*segment);
        return _spuModules.size() - 1;
    }
    
    inline void invokePPU(unsigned index, unsigned label) {
        _ppuModules[index].ppuEntryPoint(g_state.th, label);
    }
    
    inline void invokeSPU(unsigned index, unsigned label, uint32_t cia) {
        _spuModules[index].spuEntryPoint(g_state.sth, label, cia);
    }
};

class ELFLoader {
    std::string _elfName;
    std::vector<uint8_t> _file;
    Elf64_be_Ehdr* _header;
    Elf64_be_Phdr* _pheaders;
    Elf64_be_Shdr* _sections;
    Elf64_be_Shdr* findSectionByName(std::string name);
    void foreachGlobalSymbol(std::function<void(Elf64_be_Sym*)> action);
    module_info_t* _module = nullptr;
    std::tuple<prx_import_t*, int> elfImports();
    std::tuple<prx_import_t*, int> prxImports();
    
public:
    ELFLoader();
    ~ELFLoader();
    uint64_t entryPoint();
    const char* getString(uint32_t idx);
    const char* getSectionName(uint32_t idx);
    Elf64_be_Sym* getGlobalSymbolByValue(uint32_t value, uint32_t section);
    uint32_t getSymbolValue(std::string name);
    void load(std::string filePath);
    std::vector<StolenFuncInfo> map(make_segment_t makeSegment,
                                    ps3_uintptr_t imageBase,
                                    std::vector<std::string> x86paths,
                                    RewriterStore* store,
                                    bool rewriter);
    void link(std::vector<std::shared_ptr<ELFLoader>> prxs);
    ThreadInitInfo getThreadInitInfo();
    std::string elfName();
    std::string shortName();
    module_info_t* module();
    Elf64_be_Ehdr* header();
    Elf64_be_Phdr* pheaders();
    std::tuple<prx_export_t*, int> exports();
    std::tuple<prx_import_t*, int> imports();
};

uint32_t findExportedSymbol(std::vector<std::shared_ptr<ELFLoader>> const& prxs,
                            uint32_t id,
                            std::string library,
                            prx_symbol_type_t type);
