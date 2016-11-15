#include <QProcess>

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
