#pragma once

#include "../sys.h"
#include "../../spu/SPUThread.h"
#include <string>
#include <array>
#include <functional>
#include <stdint.h>

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

class SpuImage {
    std::array<uint8_t, LocalStorageSize> _ls;
    std::string _desc;
    uint32_t _ep;
    uint32_t _src;

public:
    SpuImage(std::function<void(uint32_t, void*, size_t)> read, ps3_uintptr_t src);
    uint8_t* localStorage();
    uint32_t entryPoint();
    uint32_t source();
};
