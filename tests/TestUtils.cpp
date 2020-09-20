#include <QProcess>
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/utils.h"
#include "ps3emu/utils/ranges.h"
#include "ps3emu/build-config.h"
#include "ps3emu/Config.h"
#include "TestUtils.h"
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <catch2/catch.hpp>
#include <unistd.h>

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

    auto runnerPath = (std::filesystem::path(g_ps3bin) / "ps3run/ps3run").string();
    auto argsStr = sformat(
        "{} {}",
        runnerPath,
        boost::join(list | ranges::views::transform(&QString::toStdString), " "));

    proc.start(runnerPath.c_str(), list);
    if (!proc.waitForStarted())
        throw std::runtime_error("can't start ps3run: " + argsStr);
    if (!proc.waitForFinished(180 * 1000))
        throw std::runtime_error("ps3run timed out: " + argsStr);
    if (proc.exitCode())
        throw std::runtime_error("ps3run exited with error: " + argsStr);
    return QString(proc.readAll()).toStdString();
}

void rewrite_and_compile(std::string elf, std::string cppPath) {
    auto line = rewrite(elf, cppPath, "");
    std::string output;
    if (!exec(line, output)) {
        FAIL(sformat("failed to exec: {}", line));
    }
    line = compile(cppPath + ".ninja");
    if (!exec(line, output)) {
        FAIL(sformat("failed to exec: {}", line));
    }
}

void rewrite_and_compile_spu(std::string elf, std::string cppPath) {
    auto line = rewrite(elf, cppPath, "--spu");
    std::string output;
    if (!exec(line, output)) {
        FAIL(sformat("failed to exec: {}", line));
    }
    line = compile(cppPath + ".ninja");
    if (!exec(line, output)) {
        FAIL(sformat("failed to exec: {}", line));
    }
}

void test_interpreter_and_rewriter(std::vector<std::string> args,
                                   std::string expected,
                                   bool rewriterOnly,
                                   std::vector<std::string> ps3runArgs) {
    auto id = getpid();
    auto spuSoPath = sformat("{}/ps3_{}{}.so", getTestOutputDir(), id, g_rewriterSpuExtension);
    auto spuCppPath = sformat("{}/ps3_{}{}", getTestOutputDir(), id, g_rewriterSpuExtension);
    auto soPath = sformat("{}/ps3_{}{}.so", getTestOutputDir(), id, g_rewriterPpuExtension);
    auto cppPath = sformat("{}/ps3_{}{}", getTestOutputDir(), id, g_rewriterPpuExtension);
    if (!rewriterOnly) {
        auto output = startWaitGetOutput(args);
        REQUIRE( output == expected );
    }
    rewrite_and_compile(args[0], cppPath);
    rewrite_and_compile_spu(args[0], spuCppPath);
    ps3runArgs.push_back("--x86");
    ps3runArgs.push_back(soPath);
    ps3runArgs.push_back("--x86");
    ps3runArgs.push_back(spuSoPath);
    auto output = startWaitGetOutput(args, ps3runArgs);
    REQUIRE( output == expected );
}

std::string testPath(const char* relative) {
    return sformat("{}/tests/binaries/{}", g_ps3sources, relative);
}
