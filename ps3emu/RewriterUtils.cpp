#include "RewriterUtils.h"

#include "ps3emu/utils.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>

std::string ps3toolPath() {
    return ssnprintf("%s/ps3tool/ps3tool", getenv("PS3_BIN"));
}

std::string rewrite(std::string elf, std::string cpp, std::string args) {
    auto line = ssnprintf("%s rewrite --elf %s --cpp %s %s",
                          ps3toolPath(),
                          boost::filesystem::absolute(elf).string(),
                          cpp,
                          args);
    return line;
}

std::string compileRule() {
    auto include = getenv("PS3_INCLUDE");
    auto lib = std::string(getenv("PS3_BIN")) + "/ps3emu";
    auto memoryProtection =
#ifdef MEMORY_PROTECTION
        "-DMEMORY_PROTECTION"
#else
        ""
#endif
    ;
    auto line = ssnprintf("g++ -shared -fPIC -std=c++17 %s $opt $trace -march=native -isystem%s -L%s $in -lps3emu -o $out",
        memoryProtection, include, lib
    );
    return line;
}

std::string compile(CompileInfo const& info) {
    auto optimization = info.debug || static_debug ? "-O0 -ggdb" : "-O3 -fno-strict-aliasing -DNDEBUG"; // -flto
    auto trace = info.trace ? "-DTRACE" : "";
    auto rule = compileRule();
    boost::replace_first(rule, "$opt", optimization);
    boost::replace_first(rule, "$trace", trace);
    boost::replace_first(rule, "$in", info.cpp);
    boost::replace_first(rule, "$out", info.so);
    return rule;
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
