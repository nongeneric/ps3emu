#include "ps3tool.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include <fstream>
#include <vector>
#include <iostream>

void HandleParseSpursTrace(ParseSpursTraceCommand const& command) {
    std::ifstream f(command.dump);
    if (!f.is_open()) {
        throw std::runtime_error("can't read dump file");
    }
    f.seekg(0, std::ios_base::end);
    auto file_size = f.tellg();
    std::vector<char> dump(file_size);
    f.seekg(0, std::ios_base::beg);
    f.read(dump.data(), dump.size());
    
    dumpSpursTrace([&](auto const& str) { std::cout << str << "\n"; }, &dump[0], dump.size());
}
