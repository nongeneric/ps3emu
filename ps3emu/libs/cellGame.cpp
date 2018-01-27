#include "cellGame.h"

#include "../ContentManager.h"
#include "../MainMemory.h"
#include "ps3emu/InternalMemoryManager.h"
#include <boost/filesystem.hpp>
#include "../log.h"
#include "ps3emu/state.h"

using namespace boost::filesystem;

namespace {
    void init(CellGameContentSize* size, Process* proc) {
        auto gb2 = 2 * 1024 * 1024;
        auto cm = g_state.content;
        auto host = cm->toHost(cm->contentDir().c_str());
        size->hddFreeSizeKB = space(host).available / 1024;
        size->sizeKB = -1; // not calculated
        size->sysSizeKB = gb2;
    }
}

int32_t cellGamePatchCheck(CellGameContentSize *size, uint64_t reserved) {
    assert(false && "should never be called");
    return CELL_OK;
}

int32_t cellGameContentPermit(cell_game_path_t* contentPath, cell_game_path_t* usrdirPath, Process* proc) {
    INFO(libs) << __FUNCTION__;
    auto usrDir = g_state.content->usrDir();
    auto contentDir = g_state.content->contentDir();
    std::copy(begin(usrDir), end(usrDir), begin(*usrdirPath));
    std::copy(begin(contentDir), end(contentDir), begin(*contentPath));
    return CELL_OK;
}

int32_t cellGameGetParamString(uint32_t id, ps3_uintptr_t buf, uint32_t bufsize, Process* proc) {
    INFO(libs) << __FUNCTION__;
    auto sfo = g_state.content->sfo();
    auto entry = std::find_if(begin(sfo), end(sfo), [=] (auto& entry) {
        return entry.id == id;
    });
    if (entry == end(sfo)) {
        assert(bufsize > 0);
        g_state.mm->setMemory(buf, 0, 1);
        return CELL_OK;
    }
    assert(entry->id != (uint32_t)-1);
    auto data = boost::get<std::string>(entry->data);
    assert(bufsize >= data.size());
    g_state.mm->writeMemory(buf, data.c_str(), data.size());
    return CELL_OK;
}

int32_t cellGameBootCheck(big_uint32_t* type,
                          big_uint32_t* attributes,
                          CellGameContentSize* size,
                          cell_game_dirname_t* dirName,
                          Process* proc)
{
    INFO(libs) << __FUNCTION__;
    init(size, proc);
    *type = CELL_GAME_GAMETYPE_DISC;
    *attributes = 0;
    auto str = "EMUGAME";
    if (dirName) {
        memcpy(dirName, str, strlen(str) + 1);
    }
    return CELL_OK;
}

int32_t cellGameDataCheck(uint32_t type, 
                          const cell_game_dirname_t* dirName, 
                          CellGameContentSize* size,
                          Process* proc)
{
    INFO(libs) << ssnprintf("cellGameDataCheck(%d, %s, ...)", type, dirName);
    init(size, proc);
    return CELL_OK;
}

int32_t cellGameDataCheckCreate2(uint32_t type,
                                 const cell_game_dirname_t *dirName,
                                 uint32_t arg,
                                 const fdescr* cb,
                                 uint32_t memContainer,
                                 PPUThread* th,
                                 boost::context::continuation* sink)
{
    uint32_t va;
    auto size = g_state.memalloc->internalAllocU<4, CellGameContentSize>(&va);
    cellGameDataCheck(type, dirName, size.get(), g_state.proc);
    th->ps3call(*cb, {(uint64_t)arg, (uint64_t)va}, sink);
    return CELL_OK;
}
