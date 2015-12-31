#include "cellGame.h"

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

int32_t cellGamePatchCheck(CellGameContentSize *size, uint64_t reserved) {
    assert(false && "should never be called");
    return CELL_OK;
}

int32_t cellGameContentPermit(cell_game_path_t* contentPath, cell_game_path_t* usrdirPath, Process* proc) {
    auto usrDir = proc->contentManager()->usrDir();
    auto contentDir = proc->contentManager()->contentDir();
    std::copy(begin(usrDir), end(usrDir), begin(*usrdirPath));
    std::copy(begin(contentDir), end(contentDir), begin(*contentPath));
    return CELL_OK;
}

int32_t cellGameGetParamString(uint32_t id, ps3_uintptr_t buf, uint32_t bufsize, Process* proc) {
    auto sfo = proc->contentManager()->sfo();
    auto entry = std::find_if(begin(sfo), end(sfo), [=] (auto& entry) {
        return entry.id == id;
    });
    if (entry == end(sfo)) {
        assert(bufsize > 0);
        proc->mm()->setMemory(buf, 0, 1);
        return CELL_OK;
    }
    assert(entry->id != (uint32_t)-1);
    auto data = boost::get<std::string>(entry->data);
    assert(bufsize >= data.size());
    proc->mm()->writeMemory(buf, data.c_str(), data.size());
    return CELL_OK;
}

int32_t cellGameBootCheck(big_uint32_t* type,
                          big_uint32_t* attributes,
                          CellGameContentSize* size,
                          cell_game_dirname_t* dirName)
{
    auto gb2 = 2 * 1024 * 1024;
    size->hddFreeSizeKB = gb2;
    size->sizeKB = gb2;
    size->sysSizeKB = gb2;
    *type = CELL_GAME_GAMETYPE_HDD;
    *attributes = 0;
    auto str = "EMUGAME";
    memcpy(dirName, str, strlen(str) + 1);
    return CELL_OK;
}
