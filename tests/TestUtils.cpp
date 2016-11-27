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
    proc.waitForStarted();
    proc.waitForFinished();
    return QString(proc.readAll()).toStdString();
}

bool rewrite_and_compile(std::string elf) {
    std::string output;
    auto res = rewrite(elf, "/tmp/x86.cpp", "", output);
    if (!res)
        return false;
    return compile({"/tmp/x86.cpp", "/tmp/x86.so", false, false});
}
