/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2009 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#ifndef __GAME_DATA_H__
#define __GAME_DATA_H__

#include <sys/types.h>

#include <sysutil/sysutil_gamecontent.h>

class GameData {
 public:
    GameData(const char* dirName);
    ~GameData();

    int initialize();
    int finalize();

    const char* getGameDataPath();

 private:
    static const int kDirNameLength_ = 64;

 private:
    char mDirName_[kDirNameLength_];
    char mContentInfoPath_[CELL_GAME_PATH_MAX];
    char mUserdirPath_[CELL_GAME_PATH_MAX];
};

#endif /* __GAME_DATA_H__ */
