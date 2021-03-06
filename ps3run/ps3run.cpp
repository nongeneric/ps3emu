#include "ps3emu/Process.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include "ps3emu/EmuCallbacks.h"
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "stdio.h"
#include <signal.h>
#include <iostream>

using namespace boost::program_options;

class RuntimeEmuCallbacks : public IEmuCallbacks {
public:
    virtual void stdout(const char* str, int len) override {
        fwrite(str, 1, len, ::stdout);
        fflush(::stdout);
    }

    virtual void stderr(const char* str, int len) override {
        fwrite(str, 1, len, ::stderr);
        fflush(::stderr);
    }

    virtual void spustdout(const char* str, int len) override {
        stdout(str, len);
    }
};


void emulate(std::string path, std::vector<std::string> args) {
    RuntimeEmuCallbacks callbacks;
    g_state.callbacks = &callbacks;

    Process proc;
    proc.init(path, args);
    for (;;) {
        auto untyped = proc.run();
        if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            ERROR(libs) << sformat("invalid instruction at {:x}", ev->thread->getNIP());
            return;
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            return;
        }
    }
}

void sigsegv_handler(int sig) {
    ERROR(libs) << sformat("crash (signal {}):\n{}", sig, print_backtrace());
    _exit(1);
}

void sigint_handler(int sig) {
    _exit(0);
}

int main(int argc, char* argv[]) {
    g_state.init();
    std::string elfPath, elfArgs, filter, sinks, format;
    bool captureRsx;
    options_description consoleDescr("Allowed options");
    try {
        consoleDescr.add_options()
            ("help", "produce help message")
            ("elf,e", value<std::string>(&elfPath)->required(), "elf file")
            ("args,a", value<std::string>(&elfArgs)->default_value(""), "elf arguments")
            ("filter,f", value<std::string>(&filter)->default_value(""),
                "logging filter: W,Drsx,Ilibs")
            ("sinks,s", value<std::string>(&sinks)->default_value(""),
                "logging sinks: file, console [e.g. file,console]")
            ("format", value<std::string>(&format)->default_value("simple"),
                "logging format: date, simple")
            ("x86", value<std::vector<std::string>>(&g_state.config->x86Paths),
                "rewritten and compiled x86 so file")
            ("capture-rsx", bool_switch()->default_value(false), "capture rsx")
            ("capture-audio", bool_switch()->default_value(false), "capture audio")
            ;
        variables_map console_vm;
        store(parse_command_line(argc, argv, consoleDescr), console_vm);
        if (console_vm.count("help")) {
            std::cout << consoleDescr;
            return 0;
        }
        notify(console_vm);
        captureRsx = console_vm["capture-rsx"].as<bool>();
        g_state.config->captureAudio = console_vm["capture-audio"].as<bool>();
        
    } catch(std::exception& e) {
        std::cout << "can't parse program options:\n";
        std::cout << e.what() << "\n\n";
        std::cout << consoleDescr;
        return 1;
    }
    
    log_init(log_parse_sinks(sinks),
             filter,
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
        ERROR(libs) << e.what();
        return 1;
    }
    return 0;
}
