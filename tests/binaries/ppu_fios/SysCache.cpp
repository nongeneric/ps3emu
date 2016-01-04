/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SysCache.h"


SysCache::SysCache(const char* cacheId) {
    strncpy(mSysCacheParam_.cacheId, cacheId, strlen(cacheId) + 1);
}

SysCache::~SysCache() {}

int SysCache::initialize() {
    mSysCacheParam_.reserved = 0;
    int result = cellSysCacheMount(&mSysCacheParam_);

    if(result != CELL_SYSCACHE_RET_OK_CLEARED &&
       result != CELL_SYSCACHE_RET_OK_RELAYED) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(-1);
    }

    printf("SysCache : %s\n", getCachePath());

    return 0;
}

int SysCache::finalize() {
    return 0;
}

const char* SysCache::getCachePath() {
    return mSysCacheParam_.getCachePath;
}
