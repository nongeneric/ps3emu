#include "RewriterUtils.h"

#include "ps3emu/utils.h"
#include "ps3emu/log.h"
#include "ps3emu/build-config.h"
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>

using namespace std::filesystem;

std::string ps3toolPath() {
    return ssnprintf("%s/ps3tool/ps3tool", g_ps3bin);
}

std::string rewrite(std::string elf, std::string cpp, std::string args) {
    auto line = ssnprintf("%s rewrite --elf %s --cpp %s %s",
                          ps3toolPath(),
                          absolute(elf).string(),
                          cpp,
                          args);
    return line;
}

std::string compileRule() {
    auto include = getenv("PS3_INCLUDE");
    auto memoryProtection =
#ifdef MEMORY_PROTECTION
        "-DMEMORY_PROTECTION"
#else
        ""
#endif
    ;
    auto line = ssnprintf("%s -c -fPIC -std=c++17 %s $opt $trace -march=native -isystem%s $in -o $out",
        g_compiler, memoryProtection, include
    );
    return line;
}

std::string linkRule() {
    auto lib = std::string(g_ps3bin) + "/ps3emu";
    return ssnprintf("%s -shared $in -L%s -lps3emu -o $out", g_compiler, lib);
}

std::string compile(std::string ninja) {
    auto wd = path(ninja).parent_path().string();
    return ssnprintf("ninja-build -C %s -f %s", wd, ninja);
}

bool exec(std::string line, std::string& output) {
    auto p = popen(line.c_str(), "r");
    output = "";
    char buf[100];
    while (fgets(buf, sizeof buf, p)) {
        output += buf;
    }
    return pclose(p) == 0;
}
