#include "SpuImage.h"
#include "../../log.h"

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

SpuImage::SpuImage(std::function<void(uint32_t, void*, size_t)> read,
                   ps3_uintptr_t src)
    : _src(src) {
    Elf32_be_Ehdr header;
    read(src, &header, sizeof(header));

    _ep = header.e_entry;

    assert(sizeof(Elf32_Phdr) == header.e_phentsize);
    std::vector<Elf32_be_Phdr> phs(header.e_phnum);
    read(src + header.e_phoff, &phs[0], phs.size() * sizeof(Elf32_Phdr));

    memset(&_ls[0], 0, _ls.size());
    for (auto& ph : phs) {
        assert(ph.p_type != PT_LOAD || ph.p_type != PT_NOTE);
        if (ph.p_type == PT_NOTE) {
            std::vector<char> vec(ph.p_filesz);
            read(src + ph.p_offset, &vec[0], ph.p_filesz);
            _desc = std::string(&vec[20]);
            continue;
        }
        if (ph.p_vaddr % ph.p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        if (ph.p_memsz == 0)
            continue;

        LOG << ssnprintf("mapping segment of size %" PRIx64 " to %" PRIx64
                         "-%" PRIx64,
                         (uint64_t)ph.p_filesz,
                         (uint64_t)ph.p_vaddr,
                         (ph.p_paddr + ph.p_memsz));

        assert(ph.p_memsz >= ph.p_filesz);
        read(src + ph.p_offset, &_ls[ph.p_vaddr], ph.p_filesz);
    }
}

uint8_t* SpuImage::localStorage() {
    return &_ls[0];
}

uint32_t SpuImage::entryPoint() {
    return _ep;
}

uint32_t SpuImage::source() {
    return _src;
}