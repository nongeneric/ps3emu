#include "MainWindow.h"

#include "ps3emu/log.h"
#include "ps3emu/libs/spu/sysSpu.h"
#include "Config.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include <boost/program_options.hpp>
#include <string>
#include <stdint.h>
#include <QApplication>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    g_config.load();
    log_init(log_file | log_console,
             log_info,
             (g_config.config().LogSpu ? log_spu : 0) |
                 (g_config.config().LogRsx ? log_rsx : 0) |
                 (g_config.config().LogLibs ? log_libs : 0)
                 | log_debugger,
             log_trace | log_cache | (g_config.config().LogSync ? log_sync : 0),
             g_config.config().LogDates ? log_date : log_simple);
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
        auto list = QString::fromStdString(arguments).split(' ', QString::SkipEmptyParts);
        w.loadElf(QString::fromStdString(elfPath), list);
    }
    w.show();
    return app.exec();
}
