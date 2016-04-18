#include "../ps3emu/Process.h"
#include "../ps3emu/ppu/ppu_dasm.h"
#include "stdio.h"
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/trivial.hpp>

using namespace boost::log;

void emulate(const char* path, std::vector<std::string> args) {
    Process proc;
    proc.init(path, args);
    for (;;) {
        auto untyped = proc.run();
        if (auto ev = boost::get<PPUInvalidInstructionEvent>(&untyped)) {
            BOOST_LOG_TRIVIAL(error)
                << ssnprintf("invalid instruction at %x", ev->thread->getNIP());
            return;
        } else if (boost::get<ProcessFinishedEvent>(&untyped)) {
            return;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("specify elf path\n");
        return 1;
    }
    
    core::get()->set_logging_enabled(false);
    
//     add_file_log(
//         keywords::file_name = "/tmp/ps3run.log",
//         keywords::auto_flush = true
//     );
    
//     core::get()->set_filter
//     (
//         trivial::severity >= trivial::info
//     );
    
    auto path = argv[1];
    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        emulate(path, args);
    } catch(std::exception& e) {
        return 1;
    }
    return 0;
}