#pragma once

#include "sys_defs.h"
#include "../ps3emu/Process.h"
#include <array>

struct CellGameContentSize {
    big_int32_t hddFreeSizeKB;
    big_int32_t sizeKB;
    big_int32_t sysSizeKB;
};
static_assert(sizeof(CellGameContentSize) == 12, "");

constexpr uint32_t CELL_GAME_ATTRIBUTE_PATCH = 1<<0;
constexpr uint32_t CELL_GAME_ATTRIBUTE_APP_HOME = 1<<1;
constexpr uint32_t CELL_GAME_ATTRIBUTE_DEBUG = 1<<2;
constexpr uint32_t CELL_GAME_ATTRIBUTE_XMBBUY = 1<<3;
constexpr uint32_t CELL_GAME_ATTRIBUTE_COMMERCE2_BROWSER = 1<<4;
constexpr uint32_t CELL_GAME_ATTRIBUTE_INVITE_MESSAGE = 1<<5;
constexpr uint32_t CELL_GAME_ATTRIBUTE_CUSTOM_DATA_MESSAGE = 1<<6;
constexpr uint32_t CELL_GAME_ATTRIBUTE_WEB_BROWSER = 1<<8;

enum CellGameGameType {
    CELL_GAME_GAMETYPE_SYS = 0,
    CELL_GAME_GAMETYPE_DISC,
    CELL_GAME_GAMETYPE_HDD,
    CELL_GAME_GAMETYPE_GAMEDATA,
    CELL_GAME_GAMETYPE_HOME
};

enum CellGameParamSize {
    CELL_GAME_PATH_MAX = 128,
    CELL_GAME_DIRNAME_SIZE          = 32,
    CELL_GAME_TITLE_SIZE            = 128,
    CELL_GAME_TITLEID_SIZE          = 10,
    CELL_GAME_HDDGAMEPATH_SIZE      = 128,
    CELL_GAME_THEMEFILENAME_SIZE    = 48
};

using cell_game_path_t = std::array<char, CELL_GAME_PATH_MAX>;
static_assert(sizeof(cell_game_path_t) == CELL_GAME_PATH_MAX, "");
using cell_game_dirname_t = std::array<char, CELL_GAME_DIRNAME_SIZE>;
static_assert(sizeof(cell_game_dirname_t) == CELL_GAME_DIRNAME_SIZE, "");

int32_t cellGameBootCheck(big_uint32_t *type,
                          big_uint32_t *attributes,
                          CellGameContentSize *size,
                          cell_game_dirname_t* dirName);
int32_t cellGamePatchCheck(CellGameContentSize *size, uint64_t reserved);
int32_t cellGameContentPermit(cell_game_path_t *contentInfoPath, cell_game_path_t *usrdirPath, Process* proc);
int32_t cellGameGetParamString(uint32_t id, ps3_uintptr_t buf, uint32_t bufsize, Process* proc);
