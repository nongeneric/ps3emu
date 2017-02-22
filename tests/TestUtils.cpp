#include <QProcess>
#include "ps3emu/RewriterUtils.h"
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
