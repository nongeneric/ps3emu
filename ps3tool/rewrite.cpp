#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/utils.h"

#include <boost/endian/arithmetic.hpp>
#include <iostream>

using namespace boost::endian;

void HandleRewrite(RewriteCommand const& command) {
    ELFLoader elf;
    MainMemory mm;
    elf.load(command.elf);
    uint32_t vaBase = 0;
    elf.map(&mm, [&](auto va, auto size, auto index) { if (vaBase == 0) vaBase = va; });
    
    for (auto i = 0x126dc; i != 0x1440c; i += 4) {
        big_uint32_t instr = mm.load32(i);
        std::string rewritten, printed;
        ppu_dasm<DasmMode::Rewrite>(&instr, i, &rewritten);
        ppu_dasm<DasmMode::Print>(&instr, i, &printed);
        std::cout << ssnprintf("_0x%x:\t%s; // %s\n", i, rewritten, printed);
    }
}
