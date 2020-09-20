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
        auto path = sformat("/tmp/blob_{}_{}.bin", command.frame, command.command);
        write_all_bytes(&gcmCommand.blob[0], gcmCommand.blob.size(), path);
        std::cout << sformat("blob saved to {}\n", path);
        return;
    }

    for (auto frame = 0; frame < db.frames(); ++frame) {
        std::cout << sformat("# frame {}\n", frame);
        for (auto command = 0; command < db.commands(frame); ++command) {
            auto gcmCommand = db.getCommand(frame, command, true);
            auto name = printCommandId((CommandId)gcmCommand.id);
            std::string blob;
            if (!gcmCommand.blob.empty()) {
                blob = sformat(" (blob {})", gcmCommand.blob.size());
            }
            std::cout << sformat("{}{}\n", name, blob);
            for (auto& arg : gcmCommand.args) {
                std::cout << sformat("\t{} {}: 0x{:08x} ({})\n",
                                       printArgType((GcmArgType)arg.type),
                                       arg.name,
                                       arg.value,
                                       arg.value);
            }
        }
    }
}
