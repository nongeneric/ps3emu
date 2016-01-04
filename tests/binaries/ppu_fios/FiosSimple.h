/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2008 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#ifndef __FIOS_TEST_H__
#define __FIOS_TEST_H__

#include <sys/types.h>

#include <cell/fios.h>
//#include <cell/fios/compression/edge/edgezlib_decompressor.h>

#include "sample_allocator.h"
#include "spurs_init_ps3.h"
#include "SysCache.h"
#include "GameData.h"
#include "HDCache.h"

class FiosSimple {
 public:
    FiosSimple();
    ~FiosSimple();

    int initialize();
    int finalize();

 private:
    SampleAllocator mAllocator_;

    SpursInitializer* mSpursInitializer_;

    SysCache mSysCache_;
    GameData mGameData_;

    cell::fios::media*    mAppHomeMedia_;
    cell::fios::overlay*  mOverlay_;
    cell::fios::ramcache* mRAMCache_;
    cell::fios::media*    mGameDataMedia_;

    HDCache* mHDCache_;

//    cell::fios::Compression::EdgeZlibDecompressor* mEdgeZlibDecompressor_;

    cell::fios::dearchiver* mDearchiver_;
    cell::fios::scheduler*  mMainScheduler_;
    cell::fios::scheduler*  mGameDataScheduler_;
};

#endif /* __FIOS_TEST_H__ */
