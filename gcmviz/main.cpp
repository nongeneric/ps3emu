#include "MainWindowModel.h"
#include <QApplication>
#include <QMainWindow>
#include <boost/program_options.hpp>
#include <string>
#include <iostream>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    std::string tracePath;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("trace", value<std::string>(&tracePath), "trace file")
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
    MainWindowModel mainWindowModel;
    if (!tracePath.empty()) {
        mainWindowModel.loadTrace(tracePath);
    }
    mainWindowModel.window()->show();
    return app.exec();
}
