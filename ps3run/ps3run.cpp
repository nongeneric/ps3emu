#include "ps3emu/Process.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "stdio.h"
#include <signal.h>

using namespace boost::program_options;

void emulate(std::string path, std::vector<std::string> args) {
    Process proc;
    proc.init(path, args);
    for (;;) {
        auto untyped = proc.run();
        if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            INFO(libs) << ssnprintf("invalid instruction at %x", ev->thread->getNIP());
            return;
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            return;
        }
    }
}

void sigsegv_handler(int sig) {
    ERROR(libs) << ssnprintf("crash (signal %d):\n%s", sig, print_backtrace());
    exit(1);
}

void sigint_handler(int sig) {
    exit(0);
}

int main(int argc, char* argv[]) {
    std::string elfPath, elfArgs, verbosity, filter, sinks, format, area;
    bool captureRsx;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("elf,e", value<std::string>(&elfPath)->required(), "elf file")
            ("args,a", value<std::string>(&elfArgs)->default_value(""), "elf arguments")
            ("verbosity,v", value<std::string>(&verbosity)->default_value("warning"),
                "logging verbosity: info, warning, error")
            ("filter,f", value<std::string>(&filter)->default_value(""),
                "logging filter: spu, libs, ppu, debugger, perf [e.g. spu,libs]")
            ("sinks,s", value<std::string>(&sinks)->default_value(""),
                "logging sinks: file, console [e.g. file,console]")
            ("format", value<std::string>(&format)->default_value("simple"),
                "logging format: date, simple")
            ("area", value<std::string>(&area)->default_value("trace"),
                "logging area: trace, perf")
            ("x86", value<std::vector<std::string>>(&g_state.config->x86Paths),
                "rewritten and compiled x86 so file")
            ("capture-rsx", bool_switch()->default_value(false), "capture rsx")
            ;
        variables_map console_vm;
        store(parse_command_line(argc, argv, consoleDescr), console_vm);
        if (console_vm.count("help")) {
            std::cout << consoleDescr;
            return 0;
        }
        notify(console_vm);
        captureRsx = console_vm["capture-rsx"].as<bool>();
        
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << consoleDescr;
        return 1;
    }
    
    log_init(log_parse_sinks(sinks),
             log_parse_verbosity(verbosity),
             log_parse_filter(filter),
             log_parse_area(area),
             log_parse_format(format));

    signal(SIGSEGV, sigsegv_handler);
    signal(SIGINT, sigint_handler);
    
    if (captureRsx) {
        Rsx::setOperationMode(RsxOperationMode::RunCapture);
    }
    
    try {
        std::vector<std::string> argvec;
        boost::split(argvec, elfArgs, boost::is_any_of(" "), boost::token_compress_on);
        emulate(elfPath, argvec);
    } catch(std::exception& e) {
        return 1;
    }
    return 0;
}
