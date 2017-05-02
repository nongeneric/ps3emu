#include "InstrDb.h"

#include "ps3emu/utils/sqlitedb.h"
#include "ps3emu/state.h"
#include "ps3emu/Config.h"
#include "ps3emu/utils.h"
#include <boost/filesystem.hpp>
#include <assert.h>
#include <set>

using namespace sql;
using namespace boost::filesystem;

namespace {
namespace db {

struct ScalarString {
    BOOST_HANA_DEFINE_STRUCT(
        ScalarString,
        (std::string, value)
    );
};

struct ScalarInt {
    BOOST_HANA_DEFINE_STRUCT(
        ScalarInt,
        (uint64_t, value)
    );
};

struct Entry {
    BOOST_HANA_DEFINE_STRUCT(
        Entry,
        (uint64_t, id),
        (std::string, elfPath),
        (int, segment),
        (int, isPPU)
    );
};

struct Lead {
    BOOST_HANA_DEFINE_STRUCT(
        Lead,
        (uint64_t, entryId),
        (uint32_t, value)
    );
};

struct Offset {
    BOOST_HANA_DEFINE_STRUCT(
        Offset,
        (uint64_t, entryId),
        (uint32_t, value),
        (uint32_t, bytes)
    );
};

}}

static auto sqlCreate =
R"(
    CREATE TABLE IF NOT EXISTS Entry(
        ElfPath      TEXT,
        Segment      INTEGER,
        IsPPU        INTEGER,
        PRIMARY KEY(ElfPath, Segment, IsPPU)
    );
    CREATE TABLE IF NOT EXISTS Lead(
        EntryId      INTEGER,
        Value        INTEGER
    );
    CREATE TABLE IF NOT EXISTS Offset(
        EntryId      INTEGER,
        Value        INTEGER,
        Bytes        INTEGER
    );
)";

enum SQLtext {
    insert_into_entry,
    insert_into_offset,
    insert_into_lead,
    sql_text_count
};

void InstrDb::open(std::string path) {
    path = !path.empty() ? path : ssnprintf("%s/instr.db", g_state.config->configDirPath);
    _db.reset(new SQLiteDB(path, sqlCreate));
    _statements.resize(sql_text_count);
    _statements[insert_into_entry] = Statement("INSERT INTO Entry VALUES(?,?,?);", *_db);
    _statements[insert_into_offset] = Statement("INSERT INTO Offset VALUES(?,?,?)", *_db);
    _statements[insert_into_lead] = Statement("INSERT INTO Lead VALUES(?,?)", *_db);
}

std::optional<InstrDbEntry> InstrDb::findPpuEntry(std::string path) {
    auto entrySql = "SELECT oid FROM Entry WHERE ElfPath = ? AND IsPPU = 1;";
    auto entries = _db->Select<db::ScalarInt>(entrySql, path);
    if (entries.empty())
        return {};
    assert(entries.size() == 1);
    return selectEntry(entries.front().value);
}

std::optional<InstrDbEntry> InstrDb::findSpuEntry(std::string path, int totalSegment) {
    auto entrySql = "SELECT oid FROM Entry WHERE ElfPath = ? AND Segment = ? AND IsPPU = 0;";
    auto entries = _db->Select<db::ScalarInt>(entrySql, path, totalSegment);
    if (entries.empty())
        return {};
    assert(entries.size() == 1);
    return selectEntry(entries.front().value);
}


InstrDbEntry InstrDb::selectEntry(uint64_t id) {
    auto entrySql = "SELECT oid, * FROM Entry WHERE oid = ?;";
    auto offsetsSql = "SELECT * FROM Offset WHERE EntryId = ?;";
    auto leadSql = "SELECT Value FROM Lead WHERE EntryId = ?;";
    auto entries = _db->Select<db::Entry>(entrySql, id);
    if (entries.empty())
        return {};
    assert(entries.size() == 1);
    auto dbEntry = entries.front();
    auto offsets = _db->Select<db::Offset>(offsetsSql, id);
    auto leads = _db->Select<db::ScalarInt>(leadSql, id);
    InstrDbEntry entry;
    entry.id = id;
    entry.elfPath = absolute(dbEntry.elfPath).string();
    entry.segment = dbEntry.segment;
    entry.isPPU = dbEntry.isPPU;
    std::transform(begin(offsets),
                   end(offsets),
                   std::back_inserter(entry.offsets),
                   [](auto& x) { return x.value; });
    std::transform(begin(offsets),
                   end(offsets),
                   std::back_inserter(entry.offsetBytes),
                   [](auto& x) { return x.bytes; });
    std::transform(begin(leads),
                   end(leads),
                   std::back_inserter(entry.leads),
                   [](auto& x) { return x.value; });
    return entry;
}

std::vector<InstrDbEntry> InstrDb::entries() {
    std::vector<InstrDbEntry> res;
    auto entrySql = "SELECT oid FROM Entry;";
    for (auto& id : _db->Select<db::ScalarInt>(entrySql)) {
        res.push_back(selectEntry(id.value));
    }
    return res;
}

void InstrDb::insertEntry(InstrDbEntry& entry) {
    _db->Insert("BEGIN TRANSACTION");
    entry.id = _db->Insert(_statements[insert_into_entry],
                           absolute(entry.elfPath).string(),
                           entry.segment,
                           (int)entry.isPPU);
    assert(entry.offsets.size() == entry.offsetBytes.size());
    std::set<std::tuple<uint32_t, uint32_t>> offsets;
    for (auto i = 0u; i < entry.offsets.size(); ++i) {
        offsets.insert(std::tuple(entry.offsets[i], entry.offsetBytes[i]));
    }
    std::set<uint32_t> leads(begin(entry.leads), end(entry.leads));
    for (auto offset : offsets) {
        _db->Insert(_statements[insert_into_offset],
                    entry.id,
                    std::get<0>(offset),
                    std::get<1>(offset));
    }
    for (auto lead : leads) {
        _db->Insert(_statements[insert_into_lead], entry.id, lead);
    }
    _db->Insert("COMMIT");
}

void InstrDb::deleteEntry(uint64_t id) {
    auto offsetSql = "DELETE FROM Offset WHERE EntryId = ?;";
    auto leadSql = "DELETE FROM Lead WHERE EntryId = ?;";
    auto entrySql = "DELETE FROM Entry WHERE oid = ?;";
    _db->Insert(offsetSql, id);
    _db->Insert(leadSql, id);
    _db->Insert(entrySql, id);
}

InstrDb::InstrDb() = default;
InstrDb::~InstrDb() = default;
