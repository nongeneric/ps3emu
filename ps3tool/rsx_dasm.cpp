#include "ps3tool.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/utils.h"
#include <boost/endian/arithmetic.hpp>
#include <fstream>
#include <iostream>

using namespace boost::endian;

void HandleRsxDasm(RsxDasmCommand const& command) {
    std::ifstream f(command.bin);
    
    if (!f.is_open())
        throw std::runtime_error("can't open bin file");

    std::cout << "header   count prefix offset args\n";
    
    big_uint32_t raw;
    while (f.read((char*)&raw, sizeof(raw))) {
        MethodHeader header{raw};
        std::cout << ssnprintf("%08x ", raw);
        if (header.val == 0) {
            std::cout << "NOP";
            continue;
        }
        if (header.prefix.u() == 1) {
            auto offset = header.jumpoffset.u();
            std::cout << ssnprintf("JUMP %x\n", offset);
            continue;
        }
        if (header.callsuffix.u() == 2) {
            auto offset = header.calloffset.u() << 2;
            std::cout << ssnprintf("CALL %x\n", offset);
            continue;
        }
        if (header.val == 0x20000) {
            std::cout << "[RET]\n";
            continue;
        }
        std::cout << ssnprintf(
            "%-5x %-6x %-6x", header.count.u(), header.prefix.u(), header.offset.u());
        for (auto i = 0u; i < header.count.u(); ++i) {
            if (!f.read((char*)&raw, sizeof(raw))) {
                std::cout << "unexpected EOF\n";
                return;
            }
            std::cout << ssnprintf(" %08x", raw);
        }
        std::cout << "\n";
    }
}
