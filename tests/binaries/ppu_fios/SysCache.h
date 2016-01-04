/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#ifndef __SYS_CACHE_H__
#define __SYS_CACHE_H__

#include <sys/types.h>

#include <sysutil/sysutil_syscache.h>

class SysCache {
 public:
    SysCache(const char* cacheId);
    ~SysCache();

    int initialize();
    int finalize();

    const char* getCachePath();

 private:
    CellSysCacheParam mSysCacheParam_;
};

#endif /* __SYS_CACHE_H__ */
