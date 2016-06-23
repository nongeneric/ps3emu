#include "SpuImage.h"
#include "ps3emu/log.h"

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
