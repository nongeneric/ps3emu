#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include "../ps3emu/ppu_dasm.h"
#include "stdio.h"

void emulate(const char* path) {
    PPU ppu;
    ELFLoader elf;
    elf.load(path);
    elf.map(&ppu, [](auto){});
    elf.link(&ppu, [](auto){});
    
    try {
        for(;;) {
            uint32_t instr;
            auto cia = ppu.getNIP();
            ppu.readMemory(cia, &instr, sizeof instr);
            ppu.setNIP(cia + sizeof instr);
            ppu_dasm<DasmMode::Emulate>(&instr, cia, &ppu);
        }
    } catch (ProcessFinishedException& e) {
        return;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("specify elf path\n");
        return 1;
    }
    auto path = argv[1];
    try {
        emulate(path);
    } catch(std::exception& e) {
        printf("exception: %s\n", e.what());
        return 1;
    }
    return 0;
}