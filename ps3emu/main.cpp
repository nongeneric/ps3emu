#include "ELFLoader.h"
#include "PPU.h"

#include <stdio.h>

int main() {
    ELFLoader elf;
    elf.load("/g/ps3/reversing/simple_printf/a.elf");
//     try {
//         uint idx = 0;
//         for(;;) {
//             auto str = elf.getString(idx);
//             printf("%s\n", str.c_str());
//             idx += str.size() + 1;
//         }
//     } catch (...) { }
    
    PPU ppu;
    elf.map(&ppu);
    elf.link(&ppu);
    ppu.run();
}
