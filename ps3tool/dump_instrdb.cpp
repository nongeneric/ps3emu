#include "ps3tool.h"
#include "ps3emu/utils.h"
#include "ps3emu/execmap/InstrDb.h"
#include <iostream>

void HandleDumpInstrDb(DumpInstrDbCommand const& command) {
    InstrDb db;
    db.open();
    for (auto& entry : db.entries()) {
        std::cout << sformat("{} {}:{} ({} leads)\n",
                               entry.isPPU ? "PPU" : "SPU",
                               entry.elfPath,
                               entry.isPPU ? 0 : entry.segment,
                               entry.leads.size());
        for (auto i = 0u; i < entry.offsets.size(); ++i) {
            if (i != 0 && (i % 4) == 0) {
                std::cout << "\n";
            }
            auto offset = entry.offsets[i];
            auto bytes = entry.offsetBytes[i];
            std::cout << sformat("    {:8x}: {:08x}", offset, bytes);
        }
        std::cout << "\n\n";
    }
}
