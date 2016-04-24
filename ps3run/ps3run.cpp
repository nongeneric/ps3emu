#include "ps3emu/Process.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/log.h"
#include "stdio.h"

using namespace boost::log;

void emulate(const char* path, std::vector<std::string> args) {
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
    if (argc < 2) {
        printf("specify elf path\n");
        return 1;
    }
    
    log_init(true);
    
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