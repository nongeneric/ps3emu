#include "splicer.h"

#include "ps3emu/state.h"
#include "ps3emu/Process.h"
#include "ps3emu/ppu/ppu_dasm.h"
#include "ps3emu/exports/exports.h"
#include "ps3emu/utils.h"
#include "ps3emu/utils/sqlitedb.h"
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

void spliceFunction(uint32_t ea, std::function<void()> handler) {
    uint32_t instr;
    g_state.mm->readMemory(ea, &instr, 4);
    auto opcode = endian_reverse(instr) >> 26;
    if (opcode == NCALL_OPCODE)
        return;
    uint32_t segment = 0, label = 0;
    auto isBBCall = dasm_bb_call(BB_CALL_OPCODE, endian_reverse(instr), segment, label);
    auto index = addNCallEntry({ssnprintf("spliced_%x", ea), 0, [=](auto th) {
        handler();
        if (isBBCall) {
            g_state.proc->bbcall(segment, label);
        } else {
            th->setNIP(ea + 4);
            ppu_dasm<DasmMode::Emulate>(&instr, ea, th);
        }
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
