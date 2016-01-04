/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>

#include "HDCache.h"

const char* HDCache::kHDCacheDirectory_ = "FIOSCACHE";

HDCache::HDCache(SysCache* sysCache) : mSysCache_(sysCache) {}

HDCache::~HDCache() {}

int HDCache::initialize(cell::fios::media* cachedMedia, uint64_t discId,
                        bool useSingleFile, bool checkModification,
                        int32_t numBlocks, size_t blockSize)
{
    mSysCacheMedia_ = new cell::fios::ps3media(mSysCache_->getCachePath());
    if(mSysCacheMedia_ == 0) {
        printf("%s %d : Can't allocate mSysCacheMedia_\n", __FUNCTION__,
               __LINE__);
        exit(-1);
    }

    mScheduler_ = cell::fios::scheduler::createSchedulerForMedia(mSysCacheMedia_);
    if(mScheduler_ == 0) {
        printf("%s %d : Can't create mScheduler_\n", __FUNCTION__, __LINE__);
        exit(-1);
    }

    mSchedulerCache_ = new cell::fios::schedulercache(cachedMedia, mScheduler_,
                                                      kHDCacheDirectory_,
                                                      discId,
                                                      useSingleFile,
                                                      checkModification, numBlocks,
                                                      blockSize);
    if(mSchedulerCache_ == 0) {
        printf("%s %d : Can't allocate mSchedulerCache_\n", __FUNCTION__,
               __LINE__);
        exit(-1);
    }

    return CELL_OK;
}

int HDCache::finalize() {
    delete mSchedulerCache_;

    cell::fios::scheduler::destroyScheduler(mScheduler_);

    delete mSysCacheMedia_;

    return CELL_OK;
}



cell::fios::schedulercache* HDCache::GetSchedulerCache() {
    return mSchedulerCache_;
}

