/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2009 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/memory.h>

#include <sysutil/sysutil_gamecontent.h>

#include "GameData.h"

GameData::GameData(const char* dirName) {
    strncpy(mDirName_, dirName, kDirNameLength_);
}

GameData::~GameData() {}

int GameData::initialize() {
    CellGameContentSize gameContentSize;
    int result = cellGameDataCheck(CELL_GAME_GAMETYPE_GAMEDATA, mDirName_, &gameContentSize);
    if(result < CELL_OK) {
        printf("%s %d : %x\n", __FILE__, __LINE__, result);
        exit(-1);
    }

    if(result == CELL_GAME_RET_NONE) {
        CellGameSetInitParams init;
        memset(&init, 0, sizeof(init));

        strncpy(init.title, "FIOS Simple Sample", CELL_GAME_SYSP_TITLE_SIZE - 1);
        strncpy(init.titleId, mDirName_, CELL_GAME_SYSP_TITLEID_SIZE - 1);
        strncpy(init.version, "01.00", CELL_GAME_SYSP_VERSION_SIZE - 1);

        char tmpContentInfoPath[CELL_GAME_PATH_MAX];
        char tmpUserdirPath[CELL_GAME_PATH_MAX];
        result = cellGameCreateGameData(&init, tmpContentInfoPath, tmpUserdirPath);
        if(result != CELL_OK) {
            printf("%s %d : %x\n", __FILE__, __LINE__, result);
            exit(-1);
        }
    }

    result = cellGameContentPermit(mContentInfoPath_, mUserdirPath_);
    if(result != CELL_OK) {
        printf("%s %d : %x\n", __FILE__, __LINE__, result);
        exit(-1);
    }

    printf("GameData : %s\n", mUserdirPath_);

    return result;
}

int GameData::finalize() {
    return CELL_OK;
}


const char* GameData::getGameDataPath()
{
    return mUserdirPath_;
}
