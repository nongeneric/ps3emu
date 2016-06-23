#include "ps3tool.h"
#include "ps3emu/libs/spu/SpuImage.h"
#include <stdio.h>

void HandleRestoreElf(RestoreElfCommand const& command) {
    auto felf = fopen(command.elf.c_str(), "r");
    fseek(felf, 0, SEEK_END);
    std::vector<uint8_t> elfBytes(ftell(felf));
    auto elf = &elfBytes[0];
    fseek(felf, 0, SEEK_SET);
    fread(elf, 1, elfBytes.size(), felf);
    fclose(felf);

    auto header = (Elf32_be_Ehdr*)elf;
    assert(sizeof(Elf32_Phdr) == header->e_phentsize);
    auto pheaders = (Elf32_be_Phdr*)(elf + header->e_phoff);

    auto dump = fopen(command.dump.c_str(), "r");

    for (auto ph = pheaders; ph != pheaders + header->e_phnum; ++ph) {
        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_vaddr % ph->p_align != 0)
            throw std::runtime_error("complex alignment not supported");
        if (ph->p_memsz == 0)
            continue;

        assert(ph->p_memsz >= ph->p_filesz);
        fseek(dump, ph->p_vaddr, SEEK_SET);
        fread(elf + ph->p_offset, 1, ph->p_filesz, dump);
    }
    fclose(dump);

    auto output = fopen(command.output.c_str(), "w");
    fwrite(elf, 1, elfBytes.size(), output);
    fclose(output);
}
