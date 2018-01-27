#include "splicer.h"

#include "ps3emu/state.h"
#include "ps3emu/Process.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/exports/exports.h"
#include "ps3emu/utils.h"
#include "ps3emu/utils/sqlitedb.h"
#include "ps3emu/BBCallMap.h"
#include <cstdlib>

namespace {
namespace db {

struct Entry {
    BOOST_HANA_DEFINE_STRUCT(
        Entry,
        (std::string, str),
        (uint32_t, fnid)
    );
};

struct ScalarString {
    BOOST_HANA_DEFINE_STRUCT(
        ScalarString,
        (std::string, value)
    );
};

}}

void spliceFunction(uint32_t ea, std::function<void()> before, std::function<void()> after) {
    uint32_t instr;
    g_state.mm->readMemory(ea, &instr, 4);
    auto info = analyze(endian_reverse(instr), ea);
    if (info.ncall || info.flow)
        return;
    g_state.bbcallMap->set(ea, 0);
    g_state.bbcallMap->set(ea + 4, 0);
    auto index = addNCallEntry({ssnprintf("spliced_%x", ea), 0, [=](PPUThread* th) {
        wrap(std::function([=](Process* proc, PPUThread* th, MainMemory* mm, boost::context::continuation* sink) {
            before();
            ppu_dasm<DasmMode::Emulate>(&instr, ea, th);
            th->ps3call({ea + 4, th->getGPR(2)}, nullptr, 0, sink);
            after();
            return th->getGPR(3);
        }), th);
    }});
    encodeNCall(g_state.mm, ea, index);
}

std::optional<std::string> fnidToName(uint32_t fnid) {
    auto path = std::getenv("PS3_FNID_DB");
    static sql::SQLiteDB db(path, "");
    auto sql = "SELECT str FROM Fnids WHERE Fnid = ?;";
    auto rows = db.Select<db::ScalarString>(sql, fnid);
    if (rows.empty())
        return {};
    return rows.front().value;
}
