#include "MainWindowModel.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include <GLFW/glfw3.h>
#include <QApplication>
#include <QMainWindow>
#include <boost/program_options.hpp>
#include <string>
#include <iostream>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    log_init(log_console, "Irsx,Ilibs,Idebugger", log_simple);
    g_state.init();
    std::string tracePath;
    bool replay;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("trace", value<std::string>(&tracePath), "trace file")
            ("replay", bool_switch()->default_value(false), "replay trace file and exit")
            ;
        variables_map console_vm;
        store(parse_command_line(argc, argv, consoleDescr), console_vm);
        if (console_vm.count("help")) {
            std::cout << consoleDescr;
            return 0;
        }
        notify(console_vm);
        replay = console_vm["replay"].as<bool>();
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << consoleDescr;
        return 1;
    }

    if (!glfwInit()) {
        throw std::runtime_error("glfw initialization failed");
    }

    QApplication app(argc, argv);
    MainWindowModel mainWindowModel;
    if (!tracePath.empty()) {
        mainWindowModel.loadTrace(tracePath);
    }
    mainWindowModel.window()->show();
    if (replay) {
        mainWindowModel.replay();
        return 0;
    }
    return app.exec();
}
