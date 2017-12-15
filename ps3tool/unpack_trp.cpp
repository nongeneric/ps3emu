#include "ps3tool.h"
#include "ps3emu/int.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/utils.h"
#include <boost/filesystem.hpp>
#include <iostream>

using namespace boost::filesystem;

constexpr uint32_t magicValue = 0xdca24d00;

struct TrpHeader {
    big_uint32_t magic;
    big_uint32_t unk1;
    big_uint64_t fileSize;
    big_uint32_t entryCount;
    big_uint32_t entryTableOffset;
};

struct TrpFileTableEntry {
    char name[32];
    big_uint64_t offset;
    big_uint64_t size;
    big_uint32_t isConfig;
    big_uint32_t unk1;
    big_uint64_t unk2;
};

void HandleUnpackTrp(UnpackTrpCommand const& command) {
    auto file = read_all_bytes(command.trp);
    auto header = (TrpHeader*)&file[0];
    if (header->magic != magicValue) {
        std::cout << "not a trp file";
        return;
    }
    auto output = command.output;
    if (output.empty()) {
        output = command.trp + "EMU";
    }
    create_directories(output);
    auto entries = (TrpFileTableEntry*)&file[header->entryTableOffset];
    for (auto i = 0u; i < header->entryCount; ++i) {
        auto entry = &entries[i];
        std::vector<uint8_t> body(&file[entry->offset], &file[entry->offset + entry->size]);
        auto outputPath = path(output) / entry->name;
        write_all_bytes(&body[0], body.size(), outputPath.string());
    }
}
