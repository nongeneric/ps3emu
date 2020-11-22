#include "MainWindow.h"

#include "ps3emu/log.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include "Config.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include <QApplication>
#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include <stdint.h>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    g_state.init();
    g_config.load();

    std::string filter = "I,Dcache,Daudio,Dfs,Ddebugger,Dproxy";
    if (g_config.config().LogSpu) {
        filter += ",Dspu";
    }

    if (g_config.config().LogRsx) {
        filter += ",Drsx";
    }

    if (g_config.config().LogLibs) {
        filter += ",Dlibs";
    }

    if (g_config.config().LogSync) {
        filter += ",Dsync";
    }

    log_init(log_file, filter, g_config.config().LogDates ? log_date : log_simple);
    log_set_thread_name("dbg_main");
    if (g_config.config().EnableSpursTrace) {
        enableSpursTrace();
    }

    std::string elfPath, arguments;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("elf,e", value<std::string>(&elfPath), "elf file")
            ("args,a", value<std::string>(&arguments), "arguments")
            ("x86", value<std::vector<std::string>>(&g_state.config->x86Paths),
                "rewritten and compiled x86 so file")
            ;
        variables_map console_vm;
        store(parse_command_line(argc, argv, consoleDescr), console_vm);
        if (console_vm.count("help")) {
            std::cout << consoleDescr;
            return 0;
        }
        notify(console_vm);
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << consoleDescr;
        return 1;
    }

    qRegisterMetaType<uint64_t>("uint64_t");

    QApplication app(argc, argv);
    MainWindow w;
    if (!elfPath.empty()) {
        auto list = QString::fromStdString(arguments).split(' ', Qt::SkipEmptyParts);
        w.loadElf(QString::fromStdString(elfPath), list);
    }
    w.show();
    return app.exec();
}
