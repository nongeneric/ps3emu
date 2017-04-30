#include "GcmDatabase.h"

#include "sqlitedb.h"

BOOST_HANA_ADAPT_STRUCT(GcmCommandArg, value, name, type);

using namespace sql;

namespace {
namespace db {

struct ScalarInt {
    BOOST_HANA_DEFINE_STRUCT(
        ScalarInt,
        (unsigned, value)
    );
};

struct IntVector {
    BOOST_HANA_DEFINE_STRUCT(
        IntVector,
        (std::vector<uint8_t>, value)
    );
};

}}

namespace {

auto sqlCreate =
    "CREATE TABLE IF NOT EXISTS GcmCommands("
    "   Id           INTEGER,"
    "   Num          INTEGER,"
    "   Frame        INTEGER,"
    "   PRIMARY KEY(Frame, Num)"
    ");"
    "CREATE TABLE IF NOT EXISTS Args("
    "   CommandNum       INTEGER,"
    "   CommandFrame     INTEGER,"
    "   Num              INTEGER,"
    "   Name             TEXT,"
    "   Type             INTEGER,"
    "   Value            INTEGER,"
    "   PRIMARY KEY(CommandNum, CommandFrame, Num)"
    ");"
    "CREATE TABLE IF NOT EXISTS Blobs("
    "   CommandNum       INTEGER,"
    "   CommandFrame     INTEGER,"
    "   Value            BLOB,"
    "   PRIMARY KEY(CommandNum, CommandFrame)"
    ");";
    
}

GcmDatabase::GcmDatabase() = default;
GcmDatabase::~GcmDatabase() = default;

void GcmDatabase::createOrOpen(std::string path) {
    _db.reset(new SQLiteDB(path, sqlCreate));
}

void GcmDatabase::insertCommand(GcmCommand command) {
    auto sql = "INSERT INTO GcmCommands VALUES(?,?,?);";
    _db->Insert(sql, command.id, command.num, command.frame);
    auto i = 0;
    auto argSql = "INSERT INTO Args VALUES(?,?,?,?,?,?);";
    for (auto arg : command.args) {
        _db->Insert(
            argSql, command.num, command.frame, i, arg.name, arg.type, arg.value);
        i++;
    }
    auto blobSql = "INSERT INTO Blobs VALUES(?,?,?);";
    _db->Insert(blobSql, command.num, command.frame, command.blob);
}

GcmCommand GcmDatabase::getCommand(unsigned frame, unsigned num) {
    auto sqlCommand = "SELECT Id FROM GcmCommands WHERE Num = ? AND Frame = ?;";
    auto id = _db->Select<db::ScalarInt>(sqlCommand, num, frame).front().value;
    auto sqlArgs = 
        "SELECT Value, Name, Type FROM Args "
        "WHERE CommandNum = ? AND CommandFrame = ? "
        "ORDER BY Num ASC;";
    auto args = _db->Select<GcmCommandArg>(sqlArgs, num, frame);
    auto sqlBlob = 
        "SELECT Value FROM Blobs "
        "WHERE CommandNum = ? AND CommandFrame = ?;";
    auto blob = _db->Select<db::IntVector>(sqlBlob, num, frame).front().value;
    return { frame, num, id, args, blob };
}

int GcmDatabase::frames() {
    auto sql = 
        "SELECT COUNT(*) FROM"
        "    (SELECT DISTINCT Frame FROM GcmCommands);";
    return _db->Select<db::ScalarInt>(sql).front().value;
}

int GcmDatabase::commands(int frame) {
    auto sql = "SELECT COUNT(*) FROM GcmCommands WHERE Frame = ?;";
    return _db->Select<db::ScalarInt>(sql, frame).front().value;
}

const char* printArgType(GcmArgType type) {
    switch (type) {
        case GcmArgType::Bool: return "bool";
        case GcmArgType::Float: return "float";
        case GcmArgType::None: return "none";
        case GcmArgType::UInt8: return "uint8_t";
        case GcmArgType::UInt16: return "uint16_t";
        case GcmArgType::UInt32: return "uint32_t";
        case GcmArgType::Int32: return "int32_t";
        case GcmArgType::Int16: return "int16_t";
    }
    assert(false); return "";
}

void GcmDatabase::close() {
    _db.reset(nullptr);
}
