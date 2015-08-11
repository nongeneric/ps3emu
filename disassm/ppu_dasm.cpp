#include "ppu_dasm.h"


std::string ftest() {
    //uint8_t instr[] = { 0x48, 0x00, 0x04, 0x49 };
    uint8_t instr[] = { 0x64, 0x42, 0x00, 0x02 };
    std::string str;
    ppu_dasm<DasmMode::Print>(instr, 0x10214, &str);
    return str;
}