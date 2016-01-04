/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#ifndef __HD_CACHE_H__
#define __HD_CACHE_H__

#include <sys/types.h>
#include <sysutil/sysutil_syscache.h>

#include <cell/fios.h>

#include "SysCache.h"

class HDCache {
 public:
    HDCache(SysCache* sysCache);
    ~HDCache();

    int initialize(cell::fios::media* cachedMedia, uint64_t discId,
                   bool useSingleFile, bool checkModification,
                   int32_t numBlocks, size_t blockSize);
    int finalize();

    cell::fios::schedulercache* GetSchedulerCache();

 private:
    static const char* kHDCacheDirectory_;

 private:
    SysCache*                   mSysCache_;
    cell::fios::media*          mSysCacheMedia_;
    cell::fios::scheduler*      mScheduler_;
    cell::fios::schedulercache* mSchedulerCache_;
};

#endif /* __HD_CACHE_H__ */
