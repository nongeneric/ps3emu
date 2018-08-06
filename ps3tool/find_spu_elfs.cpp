#include "ps3tool.h"
#include "ps3emu/utils.h"
#include "ps3emu/fileutils.h"
#include "ps3tool-core/Rewriter.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

uint32_t offsetToVa(const Elf64_be_Ehdr* header, uint32_t offset) {
    auto phs = (Elf64_be_Phdr*)((char*)header + header->e_phoff);
    for (auto i = 0; i < header->e_phnum; ++i) {
        auto offsetStart = phs[i].p_offset;
        auto offsetEnd = offsetStart + phs[i].p_filesz;
        if (offsetStart <= offset && offset < offsetEnd)
            return offset - offsetStart + phs[i].p_vaddr;
    }
    throw std::runtime_error("out of range");
}

void HandleFindSpuElfs(FindSpuElfsCommand const& command) {
    auto body = read_all_bytes(command.elf);
    auto elfs = discoverEmbeddedSpuElfs(body);
    if (elfs.empty()) {
        return;
    }
    auto filename = fs::path(command.elf).stem().string();
    std::cout << ssnprintf("%d embedded elfs found in %s\n", elfs.size(), filename);
    for (auto& elf : elfs) {
        auto tmppath = ssnprintf("/tmp/%s_%x_spu.elf", filename, elf.startOffset);
        write_all_bytes(&body[elf.startOffset], elf.size, tmppath);
        std::cout << ssnprintf("    at 0x%x (va 0x%x) of size 0x%x (%d segments, %d sections, %s, %s)\n",
                               elf.startOffset,
                               offsetToVa((Elf64_be_Ehdr*)&body[0], elf.startOffset),
                               elf.size,
                               elf.header->e_phnum,
                               elf.header->e_shnum,
                               elf.isJob ? "JOB" : "REGULAR",
                               tmppath);
    }
}
