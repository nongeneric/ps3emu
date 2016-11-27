#include "RewriterUtils.h"

#include "ps3emu/utils.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>

bool rewrite(std::string elf, std::string cpp, std::string args, std::string& output) {
    auto line = ssnprintf("%s/ps3tool/ps3tool rewrite --elf %s --cpp %s %s",
                          getenv("PS3_BIN"),
                          boost::filesystem::absolute(elf).string(),
                          cpp,
                          args);
    auto p = popen(line.c_str(), "r");
    output = "";
    char buf[100];
    while (fgets(buf, sizeof buf, p)) {
        output += buf;
    }
    return pclose(p) == 0;
}

bool compile(CompileInfo const& info) {
    auto include = getenv("PS3_INCLUDE");
    auto lib = std::string(getenv("PS3_BIN")) + "/ps3emu";
    auto optimization = info.debug ? "-O0 -ggdb" : "-O3 -flto";
    auto log = info.log ? "" : "-DNDEBUG";
    auto line = ssnprintf("g++ -shared -fPIC -std=c++14 %s %s -march=native -isystem%s -L%s %s -lps3emu -o %s",
        optimization, log, include, lib, info.cpp, info.so
    );
    auto p = popen(line.c_str(), "r");
    return pclose(p) == 0;
}