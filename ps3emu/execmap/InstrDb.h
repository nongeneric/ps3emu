#pragma once

#include "ps3emu/int.h"
#include <vector>
#include <string>
#include <memory>
#include <optional>

struct InstrDbEntry {
    uint64_t id = -1;
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> offsetBytes;
    std::vector<uint32_t> leads;
    std::string elfPath;
    int segment = 0;
    bool isPPU = false;
};

namespace sql { 
    class SQLiteDB;
    class Statement;
}
class InstrDb {
    std::unique_ptr<sql::SQLiteDB> _db;
    std::vector<sql::Statement> _statements;
public:
    InstrDb();
    ~InstrDb();
    void open(std::string path = "");
    std::optional<InstrDbEntry> findPpuEntry(std::string path);
    std::vector<InstrDbEntry> findSpuEntries(std::string path);
    std::optional<InstrDbEntry> findSpuEntry(std::string path, int totalSegment);
    InstrDbEntry selectEntry(uint64_t id);
    std::vector<InstrDbEntry> entries();
    void insertEntry(InstrDbEntry& entry);
    void deleteEntry(uint64_t id);
};
