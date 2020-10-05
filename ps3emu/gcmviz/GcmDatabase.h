#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

enum class GcmArgType {
    None, UInt8, UInt16, UInt32, Int32, Int16, Float, Bool
};

struct GcmCommandArg {
    uint32_t value;
    std::string name;
    unsigned type;
};

struct GcmCommand {
    unsigned frame;
    unsigned num;
    unsigned id;
    std::vector<GcmCommandArg> args;
    std::vector<uint8_t> blob;
};

namespace sql { class SQLiteDB; }
class GcmDatabase {
    std::unique_ptr<sql::SQLiteDB> _db;
public:
    GcmDatabase();
    ~GcmDatabase();
    void createOrOpen(std::string path);
    void insertCommand(const GcmCommand& command);
    int frames();
    int commands(int frame);
    GcmCommand getCommand(unsigned frame, unsigned num, bool fillBlob);
    void close();
};

const char* printArgType(GcmArgType type);
