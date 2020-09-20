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

    std::cout << "get  header   count prefix offset args\n";

    big_uint32_t raw;
    while (f.read((char*)&raw, sizeof(raw))) {
        auto get = f.tellg();

        MethodHeader header{raw};
        std::cout << sformat("{:04x} {:08x} ", get, raw);
        if (header.value == 0) {
            std::cout << "NOP";
            continue;
        }
        if (header.prefix_u() == 1) {
            auto offset = header.jumpoffset_u();
            std::cout << sformat("JUMP {:x}\n", offset);
            continue;
        }
        if (header.callsuffix_u() == 2) {
            auto offset = header.calloffset_u() << 2;
            std::cout << sformat("CALL {:x}\n", offset);
            continue;
        }
        if (header.value == 0x20000) {
            std::cout << "[RET]\n";
            continue;
        }
        std::cout << sformat(
            "{:<5x} {:<6x} {:<6x}", header.count_u(), header.prefix_u(), header.offset_u());
        for (auto i = 0u; i < header.count_u(); ++i) {
            if (!f.read((char*)&raw, sizeof(raw))) {
                std::cout << "unexpected EOF\n";
                return;
            }
            std::cout << sformat(" {:08x}", raw);
        }
        std::cout << "\n";
    }
}
