#include "../ps3emu/PPU.h"
#include "../ps3emu/ELFLoader.h"
#include "../ps3emu/ppu_dasm.h"
#include "stdio.h"

void emulate(const char* path, std::vector<std::string> args) {
    PPU ppu;
    ELFLoader elf;
    elf.load(path);
    elf.map(&ppu, args, [](auto){});
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
    } catch (std::exception& e) {
        printf("exception: %s (NIP=%" PRIx64 ")\n", e.what(), ppu.getNIP());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("specify elf path\n");
        return 1;
    }
    auto path = argv[1];
    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        emulate(path, args);
    } catch(std::exception& e) {
        return 1;
    }
    return 0;
}