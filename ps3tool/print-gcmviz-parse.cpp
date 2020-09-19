#include "ps3tool.h"

#include "ps3emu/utils.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/gcmviz/GcmDatabase.h"
#include "ps3emu/rsx/Tracer.h"
#include <iostream>

void HandlePrintGcmVizTrace(PrintGcmVizTraceCommand const& command) {
    GcmDatabase db;
    db.createOrOpen(command.trace);

    if (command.frame != -1 && command.command != -1) {
        auto gcmCommand = db.getCommand(command.frame, command.command, true);
        auto path = ssnprintf("/tmp/blob_%d_%d.bin", command.frame, command.command);
        write_all_bytes(&gcmCommand.blob[0], gcmCommand.blob.size(), path);
        std::cout << ssnprintf("blob saved to %s\n", path);
        return;
    }

    for (auto frame = 0; frame < db.frames(); ++frame) {
        std::cout << ssnprintf("# frame %d\n", frame);
        for (auto command = 0; command < db.commands(frame); ++command) {
            auto gcmCommand = db.getCommand(frame, command, true);
            auto name = printCommandId((CommandId)gcmCommand.id);
            std::string blob;
            if (!gcmCommand.blob.empty()) {
                blob = ssnprintf(" (blob %d)", gcmCommand.blob.size());
            }
            std::cout << ssnprintf("%s%s\n", name, blob);
            for (auto& arg : gcmCommand.args) {
                std::cout << ssnprintf("\t%s %s: 0x%08x (%d)\n",
                                       printArgType((GcmArgType)arg.type),
                                       arg.name,
                                       arg.value,
                                       arg.value);
            }
        }
    }
}
