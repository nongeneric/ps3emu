#include <catch.hpp>

#include "ps3emu/execmap/InstrDb.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/utils.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>

using namespace boost::filesystem;

TEST_CASE("instrdb_simple") {
    auto path = ssnprintf("/tmp/instr_%d.db", getpid());
    if (exists(path))
        remove(path);
    
    InstrDb db;
    db.open(path);
    InstrDbEntry entry;
    entry.offsets.push_back(1);
    entry.offsets.push_back(2);
    entry.offsetBytes.push_back(10);
    entry.offsetBytes.push_back(20);
    entry.leads.push_back(3);
    entry.leads.push_back(4);
    entry.elfPath = path;
    entry.segment = 2;
    entry.isPPU = false;
    db.insertEntry(entry);
    
    auto allEntries = db.entries();
    REQUIRE(allEntries.size() == 1);
    
    auto selected = allEntries.front();
    REQUIRE(selected.offsets.size() == 2);
    REQUIRE(selected.offsets[0] == 1);
    REQUIRE(selected.offsets[1] == 2);
    REQUIRE(selected.leads[0] == 3);
    REQUIRE(selected.leads[1] == 4);
    REQUIRE(selected.leads.size() == 2);
    REQUIRE(selected.elfPath == path);
    REQUIRE(selected.segment == 2);
    REQUIRE(selected.isPPU == false);
    
    db.deleteEntry(selected.id);
    allEntries = db.entries();
    REQUIRE(allEntries.size() == 0);
}
