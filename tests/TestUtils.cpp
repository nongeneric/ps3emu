#include <QProcess>
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/utils.h"
#include "TestUtils.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <catch/catch.hpp>

static const char* runnerPath = "../ps3run/ps3run";
using namespace boost;

std::string startWaitGetOutput(std::vector<std::string> args,
                               std::vector<std::string> ps3runArgs) {
    QProcess proc;
    auto argstr = join(std::vector<std::string>(args.begin() + 1, args.end()), " ");
    QStringList list;
    for (auto& arg : ps3runArgs) {
        list << QString::fromStdString(arg);
    }
    list << "--elf" << QString::fromStdString(args.front());
    list << "--args" << QString::fromStdString(argstr);
    proc.start(runnerPath, list);
    if (!proc.waitForStarted())
        throw std::runtime_error("can't start process");
    if (!proc.waitForFinished())
        throw std::runtime_error("process timed out");
    return QString(proc.readAll()).toStdString();
}

bool rewrite_and_compile(std::string elf) {
    auto line = rewrite(elf, "/tmp/x86.cpp", "");
    std::string output;
    auto res = exec(line, output);
    if (!res)
        return false;
    line = compile({"/tmp/x86.cpp", "/tmp/x86.so", false, false});
    return exec(line, output);
}

bool rewrite_and_compile_spu(std::string elf) {
    auto line = rewrite(elf, "/tmp/x86.spu.cpp", "--spu");
    std::string output;
    auto res = exec(line, output);
    if (!res)
        return false;
    line = compile({"/tmp/x86.spu.cpp", "/tmp/x86spu.so", false, false});
    return exec(line, output);
}

void test_interpreter_and_rewriter(std::vector<std::string> args, std::string expected) {
    auto output = startWaitGetOutput(args);
    REQUIRE( output == expected );
    REQUIRE( rewrite_and_compile(args[0]) );
    REQUIRE( rewrite_and_compile_spu(args[0]) );
    output = startWaitGetOutput(args, {"--x86", "/tmp/x86.so", "--x86", "/tmp/x86spu.so"});
    REQUIRE( output == expected );
}
