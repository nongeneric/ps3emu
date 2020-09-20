#include "ps3tool.h"

#include "ps3emu/libs/spu/SpuImage.h"
#include "ps3emu/utils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/spu/SPUDasm.h"
#include <iostream>

void HandleDasm(DasmCommand const& command) {
    auto file = read_all_bytes(command.elf);
    
    std::cout << sformat("{}\n", command.elf);
    auto header = (Elf32_be_Ehdr*)&file[0];
    auto pheaders = (Elf32_be_Phdr*)(&file[header->e_phoff]);
    
    std::cout << "program headers\n";
    std::cout << "  Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Align\n";
    for (auto i = 0u; i < header->e_phnum; ++i) {
        std::cout << sformat("  0x{:06x} 0x{:08x} 0x{:08x} 0x{:05x} 0x{:05x} 0x{:x}\n",
                               pheaders[i].p_offset,
                               pheaders[i].p_vaddr,
                               pheaders[i].p_paddr,
                               pheaders[i].p_filesz,
                               pheaders[i].p_memsz,
                               pheaders[i].p_align);
    }
    
    std::cout << "\n";
    
    for (auto i = 0u; i < header->e_phnum; ++i) {
        if (pheaders[i].p_type != SYS_SPU_SEGMENT_TYPE_COPY)
            continue;
        std::cout << " === Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Align\n";
        std::cout << sformat("#=== 0x{:06x} 0x{:08x} 0x{:08x} 0x{:05x} 0x{:05x} 0x{:x}\n",
                               pheaders[i].p_offset,
                               pheaders[i].p_vaddr,
                               pheaders[i].p_paddr,
                               pheaders[i].p_filesz,
                               pheaders[i].p_memsz,
                               pheaders[i].p_align);
        for (auto ip = 0u; ip < pheaders[i].p_filesz; ip += 4) {
            auto ea = pheaders[i].p_vaddr + ip;
            auto offset = pheaders[i].p_offset + ip;
            auto instr = (uint32_t*)&file[offset];
            std::string str;
            try {
                SPUDasm<DasmMode::Print>(instr, ea, &str);
            } catch (...) {}
            std::cout << sformat("  {:8x}  {:08x}  {}\n", ea, endian_reverse(*instr), str);
        }
    }
}
