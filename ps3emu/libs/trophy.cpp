#include "trophy.h"

#include "ps3emu/state.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/log.h"
#include "ps3emu/fileutils.h"
#include "libsysutil.h"
#include <pugixml.hpp>
#include <filesystem>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <bitset>

using namespace std::filesystem;
using namespace pugi;
using namespace std::literals;

#define SCE_NP_TROPHY_STATUS_INSTALLED (3)
#define SCE_NP_TROPHY_STATUS_PROCESSING_SETUP (5)
#define SCE_NP_TROPHY_STATUS_PROCESSING_PROGRESS (6)
#define SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE (7)
#define SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE (8)

#define SCE_NP_TROPHY_GRADE_UNKNOWN (0)
#define SCE_NP_TROPHY_GRADE_PLATINUM (1)
#define SCE_NP_TROPHY_GRADE_GOLD (2)
#define SCE_NP_TROPHY_GRADE_SILVER (3)
#define SCE_NP_TROPHY_GRADE_BRONZE (4)
#define SCE_NP_TROPHY_INVALID_TROPHY_ID (-1)

namespace {
    
struct Trophy {
    int id;
    bool hidden;
    uint32_t grade;
    std::string name;
    std::string detail;
    bool unlocked;
    path imagePath;
};

struct {
    uint32_t callback = 0;
    uint32_t arg = 0;
    boost::mutex m;
    path gameImagePath;
    SceNpTrophyGameDetails gameDetails;
    SceNpTrophyGameData gameData;
    std::vector<Trophy> trophies;
} context;

void updateGameData() {
    context.gameData.unlockedTrophies = 0;
    context.gameData.unlockedPlatinum = 0;
    context.gameData.unlockedGold = 0;
    context.gameData.unlockedSilver = 0;
    context.gameData.unlockedBronze = 0;
    
    for (auto& trophy : context.trophies) {
        if (!trophy.unlocked)
            continue;
        
        context.gameData.unlockedTrophies++;
        switch (trophy.grade) {
            case SCE_NP_TROPHY_GRADE_PLATINUM: context.gameData.unlockedPlatinum++; break;
            case SCE_NP_TROPHY_GRADE_GOLD: context.gameData.unlockedGold++; break;
            case SCE_NP_TROPHY_GRADE_SILVER: context.gameData.unlockedSilver++; break;
            case SCE_NP_TROPHY_GRADE_BRONZE: context.gameData.unlockedBronze++; break;
        }
    }
}

Trophy& platinum() {
    for (auto& trophy : context.trophies) {
        if (trophy.grade == SCE_NP_TROPHY_GRADE_PLATINUM)
            return trophy;
    }
    throw std::runtime_error("no platinum id found");
}

}

std::optional<path> emuDir() {
    auto trophyDir = g_state.content->toHost(g_state.content->contentDir() + "/TROPDIR");
    if (!exists(trophyDir))
        return {};
    
    for (auto& dir : directory_iterator(trophyDir)) {
        if (is_directory(dir)) {
            for (auto& nested : directory_iterator(dir)) {
                if (!is_directory(nested))
                    continue;
                if (nested.path().filename() != "TROPHY.TRPEMU") {
                    ERROR(libs) << "TROPHY.TRPEMU dir not found";
                    return {};
                }
                return nested.path();
            }
            break;
        }
    }
    
    return {};
}

int32_t sceNpTrophyInit(uint32_t pool,
                        uint32_t poolSize,
                        uint32_t containerId,
                        uint64_t options) {
    return CELL_OK;
}

int32_t sceNpTrophyTerm(
) {
    return CELL_OK;
}

int32_t sceNpTrophyCreateContext(SceNpTrophyContext* context,
                                 uint32_t commId,
                                 uint32_t commSign,
                                 uint64_t options) {
    *context = 1;
    return CELL_OK;
}

int32_t sceNpTrophyDestroyContext(SceNpTrophyContext context) {
    return CELL_OK;
}

int32_t sceNpTrophyCreateHandle(SceNpTrophyHandle* handle) {
    return CELL_OK;
}

uint32_t parseGrade(char ch) {
    switch (ch) {
        case 'P': return SCE_NP_TROPHY_GRADE_PLATINUM;
        case 'G': return SCE_NP_TROPHY_GRADE_GOLD;
        case 'S': return SCE_NP_TROPHY_GRADE_SILVER;
        case 'B': return SCE_NP_TROPHY_GRADE_BRONZE;
        default: return SCE_NP_TROPHY_GRADE_UNKNOWN;
    }
}

int32_t sceNpTrophyRegisterContext(SceNpTrophyContext,
                                   SceNpTrophyHandle,
                                   uint32_t callback,
                                   uint32_t arg,
                                   uint64_t options,
                                   PPUThread* th,
                                   boost::context::continuation* sink) {
    context.callback = callback;
    context.arg = arg;

    auto emuPath = emuDir();
    if (!emuPath)
        return CELL_OK;
    
    auto configPath = *emuPath / "TROPCONF.SFM";
    auto tropPath = *emuPath / "TROP.SFM";

    xml_document config;
    config.load_file(configPath.string().c_str());
    for (auto t = config.child("trophyconf").child("trophy"); t; t = t.next_sibling("trophy")) {
        Trophy trophy;
        trophy.id = t.attribute("id").as_int();
        trophy.hidden = t.attribute("hidden").as_string() == "yes"s;
        trophy.grade = parseGrade(t.attribute("ttype").as_string()[0]);
        trophy.unlocked = 0;
        trophy.imagePath = *emuPath / sformat("TROP{:03}.PNG", trophy.id);
        context.trophies.push_back(trophy);
    }

    xml_document trop;
    trop.load_file(tropPath.string().c_str());
    auto trophyconf = trop.child("trophyconf");
    strncpy(context.gameDetails.title,
            trophyconf.child("title-name").text().get(),
            SCE_NP_TROPHY_NAME_MAX_SIZE - 1);
    strncpy(context.gameDetails.description,
            trophyconf.child("title-detail").text().get(),
            SCE_NP_TROPHY_DESCR_MAX_SIZE - 1);
    for (auto t = trophyconf.child("trophy"); t; t = t.next_sibling("trophy")) {
        auto id = t.attribute("id").as_int();
        auto& trophy = context.trophies.at(id);
        assert(trophy.id == id);
        trophy.name = t.child("name").text().get();
        trophy.detail = t.child("detail").text().get();
    }

    context.gameImagePath = *emuPath / "ICON0.PNG";
    context.gameDetails.numTrophies = context.trophies.size();
    context.gameDetails.numPlatinum = 0;
    context.gameDetails.numGold = 0;
    context.gameDetails.numSilver = 0;
    context.gameDetails.numBronze = 0;

    for (auto& t : context.trophies) {
        switch (t.grade) {
            case SCE_NP_TROPHY_GRADE_PLATINUM: context.gameDetails.numPlatinum++; break;
            case SCE_NP_TROPHY_GRADE_GOLD: context.gameDetails.numGold++; break;
            case SCE_NP_TROPHY_GRADE_SILVER: context.gameDetails.numSilver++; break;
            case SCE_NP_TROPHY_GRADE_BRONZE: context.gameDetails.numBronze++; break;
        }
    }

    // even though the docs specify that a call to cellSysutilCheckCallback is required to fire the events
    // this function calls the callback directly

    fdescr cbdescr;
    g_state.mm->readMemory(context.callback, &cbdescr, sizeof(cbdescr));
    int64_t res = th->ps3call(
        cbdescr, {1, SCE_NP_TROPHY_STATUS_INSTALLED, 0, 0, context.arg}, sink);
    (void)res;
    assert(res >= 0);

    th->ps3call(cbdescr,
                {1, SCE_NP_TROPHY_STATUS_PROCESSING_SETUP, 0, 0, context.arg},
                sink);
    th->ps3call(cbdescr,
                {1,
                 SCE_NP_TROPHY_STATUS_PROCESSING_PROGRESS,
                 (uint32_t)context.trophies.size(),
                 (uint32_t)context.trophies.size(),
                 context.arg},
                sink);
    th->ps3call(cbdescr,
                {1, SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE, 0, 0, context.arg},
                sink);
    th->ps3call(cbdescr,
                {1, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 0, 0, context.arg},
                sink);

    return CELL_OK;
}

#define SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM 0x80022914
#define SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED 0x80022915

int32_t sceNpTrophyUnlockTrophy(SceNpTrophyContext,
                                SceNpTrophyHandle,
                                SceNpTrophyId trophyId,
                                SceNpTrophyId* platinumId) {
    boost::lock_guard<boost::mutex> lock(context.m);
    auto& trophy = context.trophies.at(trophyId);
    assert(trophy.id == trophyId);
    auto& p = platinum();
    if (p.id == trophyId) {
        return SCE_NP_TROPHY_ERROR_CANNOT_UNLOCK_PLATINUM;
    }
    if (trophy.unlocked == 1) {
        return SCE_NP_TROPHY_ERROR_ALREADY_UNLOCKED;
    }
    trophy.unlocked = 1;
    updateGameData();
    if (context.gameData.unlockedTrophies == context.trophies.size() - 1) {
        p.unlocked = 1;
        *platinumId = p.id;
    } else {
        *platinumId = SCE_NP_TROPHY_INVALID_TROPHY_ID;
    }
    return CELL_OK;
}

int32_t sceNpTrophyGetGameInfo(SceNpTrophyContext,
                               SceNpTrophyHandle handle,
                               SceNpTrophyGameDetails* details,
                               SceNpTrophyGameData* data) {
    boost::lock_guard<boost::mutex> lock(context.m);
    updateGameData();
    if (details) {
        *details = context.gameDetails;
    }
    if (data) {
        *data = context.gameData;
    }
    return CELL_OK;
}

int32_t sceNpTrophyGetTrophyInfo(SceNpTrophyContext,
                                 SceNpTrophyHandle,
                                 SceNpTrophyId trophyId,
                                 SceNpTrophyDetails* details,
                                 SceNpTrophyData* data) {
    boost::lock_guard<boost::mutex> lock(context.m);
    auto& trophy = context.trophies.at(trophyId);
    assert(trophy.id == trophyId);
    if (details) {
        details->trophyId = trophy.id;
        details->trophyGrade = trophy.grade;
        strncpy(details->name, trophy.name.c_str(), SCE_NP_TROPHY_NAME_MAX_SIZE - 1);
        strncpy(details->description,
                trophy.detail.c_str(),
                SCE_NP_TROPHY_DESCR_MAX_SIZE - 1);
        details->hidden = trophy.hidden;
    }
    if (data) {
        data->timestamp = 0;
        data->trophyId = trophy.id;
        data->unlocked = trophy.unlocked;
    }
    return CELL_OK;
}

int32_t sceNpTrophyGetGameIcon(SceNpTrophyContext,
                               SceNpTrophyHandle,
                               uint32_t buffer,
                               big_int32_t* size) {
    INFO(libs) << sformat("sceNpTrophyGetGameIcon({}, {:x})",
                            context.gameImagePath.string(),
                            buffer);
    boost::lock_guard<boost::mutex> lock(context.m);
    if (!buffer) {
        *size = file_size(context.gameImagePath);
        return CELL_OK;
    }
    
    auto file = read_all_bytes(context.gameImagePath.string());
    g_state.mm->writeMemory(buffer, &file[0], file.size());
    
    return CELL_OK;
}

int32_t sceNpTrophyGetTrophyIcon(SceNpTrophyContext,
                                 SceNpTrophyHandle,
                                 SceNpTrophyId trophyId,
                                 big_int32_t buffer,
                                 big_int32_t* size) {
    boost::lock_guard<boost::mutex> lock(context.m);
    auto& trophy = context.trophies.at(trophyId);
    assert(trophy.id == trophyId);
    
    INFO(libs) << sformat("sceNpTrophyGetTrophyIcon({}, {:x})",
                            trophy.imagePath.string(),
                            buffer);
    
    if (!buffer) {
        *size = file_size(trophy.imagePath);
        return CELL_OK;
    }
    
    auto file = read_all_bytes(trophy.imagePath.string());
    g_state.mm->writeMemory(buffer, &file[0], file.size());
    
    return CELL_OK;
}

int32_t sceNpTrophyGetGameProgress(SceNpTrophyContext,
                                   SceNpTrophyHandle,
                                   big_int32_t* percentage) {
    boost::lock_guard<boost::mutex> lock(context.m);
    updateGameData();
    *percentage = (float)context.gameData.unlockedTrophies / context.trophies.size();
    return CELL_OK;
}

int32_t sceNpTrophyGetRequiredDiskSpace(SceNpTrophyContext,
                                        SceNpTrophyHandle,
                                        big_uint64_t* reqspace,
                                        uint64_t) {
    *reqspace = 1ull << 20ull;
    return CELL_OK;
}

int32_t sceNpTrophyGetTrophyUnlockState(SceNpTrophyContext,
                                        SceNpTrophyHandle,
                                        std::array<uint8_t, 16>* flags,
                                        big_int32_t* count) {
    boost::lock_guard<boost::mutex> lock(context.m);
    updateGameData();
    *count = context.gameData.unlockedTrophies;
    std::bitset<128> set;
    for (auto i = 0u; i < 128; ++i) {
        set[i] = context.trophies[i].unlocked;
    }
    memcpy(flags, &set, 16);
    return CELL_OK;
}

int32_t sceNpTrophyDestroyHandle(SceNpTrophyHandle handle) {
    // TODO: handle
    return CELL_OK;
}
