#include "MainWindow.h"

#include "ps3emu/log.h"
#include "Config.h"
#include <boost/program_options.hpp>
#include <string>
#include <QApplication>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    g_config.load();
    log_init(log_file | log_console,
             log_info,
             (g_config.config().LogSpu ? log_spu : 0) | log_rsx | log_libs |
                 log_debugger);
    log_set_thread_name("dbg_main");

    std::string elfPath;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("elf,e", value<std::string>(&elfPath), "elf file")
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
    
    QApplication app(argc, argv);
    MainWindow w;
    if (!elfPath.empty()) {
        w.loadElf(QString::fromStdString(elfPath), QStringList());
    }
    w.show();
    return app.exec();
}
