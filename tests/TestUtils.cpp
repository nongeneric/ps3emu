#include <QProcess>
#include "ps3emu/utils.h"
#include "TestUtils.h"
#include <boost/filesystem.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>

static const char* runnerPath = "../ps3run/ps3run";

std::string startWaitGetOutput(std::vector<std::string> args,
                               std::vector<std::string> ps3runArgs) {
    QProcess proc;
    std::string argstr;
    for (size_t i = 1; i < args.size(); ++i) {
        argstr += args[i];
        if (i != args.size() - 1)
             argstr += " ";
    }
    proc.start(runnerPath,
               QStringList() << "--elf" << QString::fromStdString(args.front())
                             << "--args" << QString::fromStdString(argstr));
    proc.waitForStarted();
    proc.waitForFinished();
    return QString(proc.readAll()).toStdString();
}

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
    auto line = ssnprintf("g++ -shared -fPIC -std=c++14 %s -march=native -isystem%s -L%s %s -lps3emu -o %s",
        optimization, include, lib, info.cpp, info.so
    );
    auto p = popen(line.c_str(), "r");
    return pclose(p) == 0;
}

bool rewrite_and_compile(std::string elf) {
    std::string output;
    auto res = rewrite(elf, "/tmp/x86.cpp", "", output);
    if (!res)
        return false;
    return compile({"/tmp/x86.cpp", "/tmp/x86.so", false, false});
}
