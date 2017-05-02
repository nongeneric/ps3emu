#include "SfoDb.h"
#include "ps3emu/int.h"
#include "ps3emu/utils/sqlitedb.h"

using namespace sql;

namespace {
namespace db {
  
struct SfoEntry {
    BOOST_HANA_DEFINE_STRUCT(
        SfoEntry,
        (std::string, key),
        (uint32_t, isInt),
        (std::string, stringValue),
        (uint32_t, intValue)
    );
};

} // db

static auto sqlCreate =
R"(
    CREATE TABLE IF NOT EXISTS SfoEntry(
        Key          TEXT,
        IsInt        INTEGER,
        StringValue  TEXT,
        IntValue     INTEGER,
        PRIMARY KEY(Key)
    );
)";

}

void SfoDb::open(std::string_view path) {
    _db.reset(new SQLiteDB(path, sqlCreate));
}

std::optional<SfoValue> SfoDb::findKey(std::string_view key) {
    auto sql = "SELECT * FROM SfoEntry WHERE Key = ?;";
    auto entries = _db->Select<db::SfoEntry>(sql, key);
    if (entries.empty())
        return {};
    auto entry = entries.front();
    if (entry.isInt)
        return entry.intValue;
    return entry.stringValue;
}

void SfoDb::setValue(std::string_view key, SfoValue value) {
    auto deleteSql = "DELETE FROM SfoEntry WHERE Key = ?;";
    auto insertSql = "INSERT INTO SfoEntry VALUES(?,?,?,?)";
    _db->Insert(deleteSql, key);
    auto isInt = std::holds_alternative<uint32_t>(value);
    _db->Insert(insertSql,
                key,
                isInt,
                isInt ? std::string() : std::get<std::string>(value),
                isInt ? std::get<uint32_t>(value) : 0u);
}

SfoDb::SfoDb() = default;
SfoDb::~SfoDb() = default;
