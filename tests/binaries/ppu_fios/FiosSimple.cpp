/*
	SCE CONFIDENTIAL
	PlayStation(R)3 Programmer Tool Runtime Library 400.001
	Copyright (C) 2009 Sony Computer Entertainment Inc.
	All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/process.h>
#include <sys/prx.h>

#include <sysutil/sysutil_syscache.h>

#include <cell/sysmodule.h>

#include "spurs_init_ps3.h"

#include "FiosSimple.h"

const char* k_TMP_TITLE_ID = "FIOSTEST";

FiosSimple::FiosSimple() : mSpursInitializer_(0), mSysCache_(k_TMP_TITLE_ID),
                           mGameData_(k_TMP_TITLE_ID), mAppHomeMedia_(0), mOverlay_(0),
                           mRAMCache_(0), mGameDataMedia_(0), mHDCache_(0),
                           /*mEdgeZlibDecompressor_(0), */mDearchiver_(0), mMainScheduler_(0),
                           mGameDataScheduler_(0) {}
FiosSimple::~FiosSimple() {}

int FiosSimple::initialize()
{
    //mSpursInitializer_ = new SpursInitializer();
    //if(mSpursInitializer_ == 0) {
    //    printf("%s %d : Can't allocate SpursInitializer()\n", __FUNCTION__, __LINE__);
    //    exit(-1);
    //}

    cell::fios::fios_parameters parameters = FIOS_PARAMETERS_INITIALIZER;

    parameters.pAllocator = &mAllocator_;
    parameters.pLargeMemcpy = 0; // Use memcpy
    parameters.pVprintf = 0;     // Use vprintf

    cell::fios::FIOSInit(&parameters);

    // Create System Cache on /dev_hdd1.
    // System Cache is used for HDCache afterwards.
    int result = mSysCache_.initialize();
    if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

    // Create media on /app_home.
    mAppHomeMedia_ = new cell::fios::ps3media(SYS_APP_HOME);
    if(mAppHomeMedia_ == 0) {
        printf("%s %d : Can't allocate mAppHomeMedia_\n", __FUNCTION__,
               __LINE__);
        exit(result);
    }

    // Create Game Data on /dev_hdd0.
    result = mGameData_.initialize();
    if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

    // Create media on /dev_hdd0(Game Data)
    mGameDataMedia_ = new cell::fios::ps3media(mGameData_.getGameDataPath());
    if(mGameDataMedia_ == 0) {
        printf("%s %d : Can't allocate mGameDataMedia_\n", __FUNCTION__,
               __LINE__);
        exit(ENOMEM);
    }

    cell::fios::scheduler_attr sched_attr = FIOS_SCHEDULER_ATTR_INITIALIZER;
    mGameDataScheduler_ =
        cell::fios::scheduler::createSchedulerForMedia(mGameDataMedia_, &sched_attr);
    if(mGameDataScheduler_ == 0) {
        printf("%s %d : Can't allocate mGameDataScheduler_\n", __FUNCTION__,
               __LINE__);
        exit(ENOMEM);
    }

    mOverlay_ = new cell::fios::overlay(mAppHomeMedia_);
    cell::fios::overlay::id_t layerId;

    // Scheduler overlay
    cell::fios::err_t err =
        mOverlay_->addLayer("/tmp0", mGameDataScheduler_, 0, "/tmp1",
                            cell::fios::overlay::kOPAQUE, &layerId);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    // Path overlay
    err = mOverlay_->addLayer("/", 0, 0, "/data/",
                              cell::fios::overlay::kDEFAULT, &layerId);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    // Create HDCache on System Cache
    mHDCache_ = new HDCache(&mSysCache_);
    if(mHDCache_ == 0) {
        printf("%s %d : Can't allocate mHDCache_\n", __FUNCTION__, __LINE__);
        exit(ENOMEM);
    }

    // If your game is multi-disc type, assign unique discID to the
    // schedulercache on each disc. The discID is used to distinguish
    // these discs.
    const uint64_t discID            = 0x6566676869707173ULL;
    const bool     useSingleFile     = true;
    const bool     checkModification = false;
    // Create 1GB HDD cache here.
    const int32_t  numBlocks         = 1024;
    const size_t   blockSize         = 1024 * 1024;
    mHDCache_->initialize(mOverlay_, discID, useSingleFile,
                          checkModification, numBlocks, blockSize);

    // Create 512KB ram cache.
    mRAMCache_ = new cell::fios::ramcache(mHDCache_->GetSchedulerCache(),
                                          16, 32 * 1024);
    if(mRAMCache_ == 0) {
        printf("%s %d : Can't allocate mRAMCache_\n", __FUNCTION__, __LINE__);
        exit(ENOMEM);
    }

    // Create EdgeZlibDecompressor for fios::dearchiver.
    //mEdgeZlibDecompressor_ = new cell::
    //    fios::Compression::EdgeZlibDecompressor(mSpursInitializer_->getSpursInstance());
    //if(mEdgeZlibDecompressor_ == 0) {
    //    printf("%s %d : Can't allocate mEdgeDecompressor_\n", __FUNCTION__,
    //           __LINE__);
    //    exit(ENOMEM);
    //}

    // Create dearchiver.
    //mDearchiver_ = new cell::fios::dearchiver(mRAMCache_,
    //                                          mEdgeZlibDecompressor_);
    //if(mDearchiver_ == 0) {
    //    printf("%s %d : Can't allocate mDearchiver_\n", __FUNCTION__, __LINE__);
    //    exit(ENOMEM);
    //}
    
    // Create main scheduler.
    mMainScheduler_ =
        cell::fios::scheduler::createSchedulerForMedia(mRAMCache_);
    if(mMainScheduler_ == 0) {
        printf("%s %d : Can't allocate mMainScheduler_\n", __FUNCTION__,
               __LINE__);
        exit(ENOMEM);
    }

    // Make mMainScheduler_ the default scheduler.
    mMainScheduler_->setDefault();

    return result;
}

int FiosSimple::finalize()
{
    cell::fios::scheduler::destroyScheduler(mMainScheduler_);

    delete mDearchiver_;
//    delete mEdgeZlibDecompressor_;
    delete mRAMCache_;

    mHDCache_->finalize();

    delete mHDCache_;
    delete mOverlay_;

    cell::fios::scheduler::destroyScheduler(mGameDataScheduler_);
    delete mGameDataMedia_;

    mGameData_.finalize();

    delete mAppHomeMedia_;

    mSysCache_.finalize();

    cell::fios::FIOSTerminate();

    delete mSpursInitializer_;

    return CELL_OK;
}

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

int main() {
    printf("FiosSimple build date : %s %s\n", __DATE__, __TIME__);
    printf("FiosSimple start.\n");

    int result;

    result = cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
    if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

    result = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
    if(result != CELL_OK) {
        printf("%s %d : %x\n", __FUNCTION__, __LINE__, result);
        exit(result);
    }

    // Construct FiosSimple instance.
    FiosSimple fiosTest;

    fiosTest.initialize();

    cell::fios::scheduler* sched = cell::fios::scheduler::getDefaultScheduler();

    // The access to "/test.psarc" is redirected to "/data/test.psarc".
    bool existTestPsarc = false;
    cell::fios::err_t err = sched->fileExistsSync(0, "/test.psarc", &existTestPsarc);
    if(err != cell::fios::CELL_FIOS_NOERROR || existTestPsarc == false) {
        printf("%s %d : %d %d\n", __FUNCTION__, __LINE__, err, existTestPsarc);
        exit(err);
    }

    cell::fios::filehandle* testPsarcFH;
    // "/test.psarc" is redirected to "/data/test.psarc".
    err = sched->openFileSync(0, "/test.psarc", cell::fios::kO_RDONLY, &testPsarcFH);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    printf("%s %d : Following files are found in \"/test.psarc\".\n", __FUNCTION__, __LINE__);

    cell::fios::direntry_t childEntry = FIOS_DIRENTRY_INITIALIZER;
    int childIndex = 0;
    // Read all files in test.psarc.
    while((err = sched->readDirectorySync(0, "/subdir", childIndex, &childEntry)) ==
          cell::fios::CELL_FIOS_NOERROR) {
        cell::fios::filehandle* patternFH;

        // Open a file in "test.psarc".
        err = sched->openFileSync(0, childEntry.fullPath, cell::fios::kO_RDONLY, &patternFH);
        if(err != cell::fios::CELL_FIOS_NOERROR) {
            printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
            exit(err);
        }

        void* buf = malloc(static_cast<size_t>(childEntry.fileSize));

        err = sched->readFileSync(0, patternFH, buf, childEntry.fileSize);
        if(err != cell::fios::CELL_FIOS_NOERROR) {
            printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
            exit(err);
        }

        free(buf);
        buf = 0;

        printf("%s : %d\n", childEntry.fullPath, (int) childEntry.fileSize);

        err = sched->closeFileSync(0, patternFH);
        if(err != cell::fios::CELL_FIOS_NOERROR) {
            printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
            exit(err);
        }

        ++childIndex;
    }


    err = sched->closeFileSync(0, testPsarcFH);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    cell::fios::opattr_t opattr = FIOS_OPATTR_INITIALIZER;

    opattr.deadline = kDEADLINE_ASAP;
    opattr.priority = cell::fios::kPRIO_DEFAULT;
    opattr.opflags = cell::fios::kOPF_CACHEPERSIST;

    cell::fios::filehandle* fh = 0;

    // Start prefetch asynchronously.
    cell::fios::op* prefetchOp = sched->prefetchFile(&opattr, "/pattern5.dat");
    if(prefetchOp == 0) {
        printf("%s %d : No operation.\n", __FUNCTION__, __LINE__);
        exit(ENOMEM);
    }


    const size_t BUF_SIZE = 1024 * 1024;

    void* buf = malloc(BUF_SIZE);

    for(unsigned int i = 0; i < BUF_SIZE; i++) {
        *(reinterpret_cast<uint8_t*>(buf) + i) = i;
    }

    err = sched->openFileSync(0, "/tmp0", cell::fios::kO_CREAT | cell::fios::kO_RDWR,
                              &fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    opattr.deadline = kDEADLINE_NOW;
    opattr.priority = cell::fios::kPRIO_DEFAULT;
    opattr.pCallback = 0;
    opattr.opflags = 0;
    opattr.pLicense = 0;

    err = sched->writeFileSync(&opattr, fh, buf, BUF_SIZE);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    err = sched->closeFileSync(0, fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    err = sched->openFileSync(0, "/tmp0", cell::fios::kO_RDONLY, &fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    opattr.deadline = kDEADLINE_ASAP;
    opattr.priority = cell::fios::kPRIO_DEFAULT;
    opattr.pCallback = 0;
    opattr.opflags = 0;
    opattr.pLicense = 0;

    err = sched->readFileSync(&opattr, fh, buf, BUF_SIZE);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    for(unsigned int i = 0; i < BUF_SIZE; i++) {
        if(*(reinterpret_cast<uint8_t*>(buf) + i) !=
           static_cast<uint8_t>(i)) {
            printf("%s %d : Data is not correct.\n", __FUNCTION__,
                   __LINE__);
            exit(-1);
        }
    }

    err = sched->closeFileSync(0, fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    err = sched->unlinkSync(0, "/tmp0");
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    free(buf);


    // Wait completion of prefetch.
    cell::fios::off_t prefetchedSize = 0;
    err = prefetchOp->syncWait(&prefetchedSize);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    printf("%s %d : prefetchedSize = %d\n", __FUNCTION__, __LINE__,
           static_cast<uint32_t>(prefetchedSize));

    system_time_t beginTime = sys_time_get_system_time();

    // Open the prefetched file.
    err = sched->openFileSync(0, "/pattern5.dat", cell::fios::kO_RDONLY, &fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    buf = malloc(static_cast<size_t>(prefetchedSize));

    // Read from the prefetched file.
    err = sched->readFileSync(0, fh, buf, prefetchedSize);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    sched->closeFileSync(0, fh);

    system_time_t endTime = sys_time_get_system_time();

    printf("It took _ to read \"pattern5.dat\" from cache.\n");


    beginTime = sys_time_get_system_time();

    err = sched->openFileSync(0, "/pattern5.dat", cell::fios::kO_RDONLY, &fh);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    opattr.deadline = kDEADLINE_ASAP;
    opattr.priority = cell::fios::kPRIO_DEFAULT;
    opattr.pCallback = 0;
    opattr.opflags = cell::fios::kOPF_NOCACHE;
    opattr.pLicense = 0;

    // Read the same file with kOPF_NOCACHE.
    err = sched->readFileSync(&opattr, fh, buf, prefetchedSize);
    if(err != cell::fios::CELL_FIOS_NOERROR) {
        printf("%s %d : %d\n", __FUNCTION__, __LINE__, err);
        exit(err);
    }

    sched->closeFileSync(0, fh);

    endTime = sys_time_get_system_time();

    printf("It took _ to read \"pattern5.dat\" without cache.\n");

    free(buf);

    fiosTest.finalize();

    printf("FiosSimple finished.\n");

    return 0;
}
