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
#include <unistd.h>

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
    if (!proc.waitForFinished(120 * 1000))
        throw std::runtime_error("process timed out");
    return QString(proc.readAll()).toStdString();
}

bool rewrite_and_compile(std::string elf, std::string cppPath) {
    auto line = rewrite(elf, cppPath, "");
    std::string output;
    auto res = exec(line, output);
    if (!res)
        return false;
    line = compile(cppPath + ".ninja");
    return exec(line, output);
}

bool rewrite_and_compile_spu(std::string elf, std::string cppPath) {
    auto line = rewrite(elf, cppPath, "--spu");
    std::string output;
    auto res = exec(line, output);
    if (!res)
        return false;
    line = compile(cppPath + ".ninja");
    return exec(line, output);
}

void test_interpreter_and_rewriter(std::vector<std::string> args, std::string expected) {
    auto id = getpid();
    auto spuSoPath = ssnprintf("/tmp/ps3_spu_%d_spu.x86.so", id);
    auto spuCppPath = ssnprintf("/tmp/ps3_spu_%d_spu", id);
    auto soPath = ssnprintf("/tmp/ps3_%d.x86.so", id);
    auto cppPath = ssnprintf("/tmp/ps3_%d", id);
    auto output = startWaitGetOutput(args);
    REQUIRE( output == expected );
    REQUIRE( rewrite_and_compile(args[0], cppPath) );
    REQUIRE( rewrite_and_compile_spu(args[0], spuCppPath) );
    output = startWaitGetOutput(args, {"--x86", soPath, "--x86", spuSoPath});
    REQUIRE( output == expected );
}
