#include "ps3emu/Process.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "stdio.h"

using namespace boost::program_options;

void emulate(std::string path, std::vector<std::string> args) {
    Process proc;
    proc.init(path, args);
    for (;;) {
        auto untyped = proc.run();
        if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            LOG << ssnprintf("invalid instruction at %x", ev->thread->getNIP());
            return;
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            return;
        }
    }
}

int main(int argc, char* argv[]) {
    std::string elfPath, elfArgs, verbosity, filter, sinks;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("elf,e", value<std::string>(&elfPath)->required(), "elf file")
            ("args,a", value<std::string>(&elfArgs)->default_value(""), "elf arguments")
            ("verbosity,v", value<std::string>(&verbosity)->default_value("warning"),
                "logging verbosity: info, warning, error")
            ("filter,f", value<std::string>(&filter)->default_value(""),
                "logging filter: spu, libs, ppu, debugger [e.g. spu,libs]")
            ("sinks,s", value<std::string>(&sinks)->default_value(""),
                "logging sinks: file, console [e.g. file,console]")
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
    
    log_init(log_parse_sinks(sinks),
             log_parse_verbosity(verbosity),
             log_parse_filter(filter),
             false);

    try {
        std::vector<std::string> argvec;
        boost::split(argvec, elfArgs, boost::is_any_of(" "), boost::token_compress_on);
        emulate(elfPath, argvec);
    } catch(std::exception& e) {
        return 1;
    }
    return 0;
}
