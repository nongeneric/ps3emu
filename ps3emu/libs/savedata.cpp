#include "libsysutil.h"
#include "savedata.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/InternalMemoryManager.h"
#include <boost/filesystem.hpp>
#include "string.h"
#include <sys/stat.h>

using namespace boost::filesystem;

struct CellSaveDataCBResult {
    big_int32_t result;
    big_uint32_t progressBarInc;
    big_int32_t errNeedSizeKB;
    big_uint32_t invalidMsg;
    big_uint32_t userdata;
};

enum CellSaveDataSystemParamSize {
    CELL_SAVEDATA_SYSP_TITLE_SIZE = 128,
    CELL_SAVEDATA_SYSP_SUBTITLE_SIZE = 128,
    CELL_SAVEDATA_SYSP_DETAIL_SIZE = 1024,
    CELL_SAVEDATA_SYSP_LPARAM_SIZE = 8
};

enum CellSaveDataParamSize {
    CELL_SAVEDATA_DIRNAME_SIZE = 32,
    CELL_SAVEDATA_FILENAME_SIZE = 13,
    CELL_SAVEDATA_SECUREFILEID_SIZE = 16,
    CELL_SAVEDATA_PREFIX_SIZE = 256,
    CELL_SAVEDATA_LISTITEM_MAX = 2048,
    CELL_SAVEDATA_SECUREFILE_MAX = 113,
    CELL_SAVEDATA_DIRLIST_MAX = 2048,
    CELL_SAVEDATA_INVALIDMSG_MAX = 256,
    CELL_SAVEDATA_INDICATORMSG_MAX = 64
};

struct CellSaveDataDirStat {
    big_int64_t _st_atime;
    big_int64_t _st_mtime;
    big_int64_t _st_ctime;
    char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
};

struct CellSaveDataSystemFileParam {
    char title[CELL_SAVEDATA_SYSP_TITLE_SIZE];
    char subTitle[CELL_SAVEDATA_SYSP_SUBTITLE_SIZE];
    char detail[CELL_SAVEDATA_SYSP_DETAIL_SIZE];
    big_uint32_t attribute;
    char reserved2[4];
    char listParam[CELL_SAVEDATA_SYSP_LPARAM_SIZE];
    char reserved[256];
};

struct CellSaveDataFileStat {
    big_uint32_t fileType;
    char reserved1[4];
    big_uint64_t st_size;
    big_uint64_t _st_atime;
    big_uint64_t _st_mtime;
    big_uint64_t _st_ctime;
    char fileName[CELL_SAVEDATA_FILENAME_SIZE];
    char reserved2[3];
};

struct CellSaveDataStatGet {
    big_int32_t hddFreeSizeKB;
    big_uint32_t isNewData;
    CellSaveDataDirStat dir;
    CellSaveDataSystemFileParam getParam;
    big_uint32_t bind;
    big_int32_t sizeKB;
    big_int32_t sysSizeKB;
    big_uint32_t fileNum;
    big_uint32_t fileListNum;
    CellSaveDataFileStat* fileList;
    char reserved[64];
};

struct CellSaveDataAutoIndicator {
    big_uint32_t dispPosition;
    big_uint32_t dispMode;
    char* dispMsg;
    big_uint32_t picBufSize;
    big_uint32_t picBuf;
    big_uint32_t reserved;
};

struct CellSaveDataStatSet {
    big_uint32_t setParam; // CellSaveDataSystemFileParam*
    big_uint32_t reCreateMode;
    big_uint32_t indicator; // CellSaveDataAutoIndicator*
};

#define CELL_SAVEDATA_BINDSTAT_OK 0
#define CELL_SAVEDATA_ERROR_CBRESULT (0x8002b401)
#define CELL_SAVEDATA_CBRESULT_OK_NEXT 0
#define CELL_SAVEDATA_CBRESULT_ERR_NOSPACE (-1)
#define CELL_SAVEDATA_CBRESULT_ERR_FAILURE (-2)
#define CELL_SAVEDATA_CBRESULT_ERR_BROKEN (-3)
#define CELL_SAVEDATA_CBRESULT_ERR_NODATA (-4)
#define CELL_SAVEDATA_CBRESULT_ERR_INVALID (-5)

int32_t cellSaveDataAutoSaveLoad(uint32_t version,
                                 cstring_ptr_t dirName,
                                 uint32_t errDialog,
                                 const CellSaveDataSetBuf* setBuf,
                                 uint32_t funcStat,
                                 uint32_t funcFile,
                                 uint32_t container,
                                 uint32_t userdata,
                                 bool isLoad) {
    INFO(libs) << ssnprintf(
        "cellSaveDataAutoSaveLoad(%s) buf: dirmax %d filemax %d size %x",
        dirName.str,
        setBuf->dirListMax,
        setBuf->fileListMax,
        setBuf->bufSize);
        
    uint32_t resultVa, getVa, setVa, setParamVa, setIndicatorVa;
    auto result = g_state.memalloc->internalAllocU<4, CellSaveDataCBResult>(&resultVa);
    auto get = g_state.memalloc->internalAllocU<4, CellSaveDataStatGet>(&getVa);
    auto set = g_state.memalloc->internalAllocU<4, CellSaveDataStatSet>(&setVa);
    auto setParam = g_state.memalloc->internalAllocU<4, CellSaveDataSystemFileParam>(&setParamVa);
    auto setIndicator = g_state.memalloc->internalAllocU<4, CellSaveDataAutoIndicator>(&setIndicatorVa);
    
    path rootPath =
        g_state.content->toHost(g_state.content->contentDir() + "/SAVEDATA/AUTO");
    create_directories(rootPath);
    auto dirPath = rootPath / dirName.str;
    
    auto isNew = !exists(dirPath);
    
    get->hddFreeSizeKB = 100ul << 20ul;
    get->isNewData = isNew;
    
    if (!isNew) {
        struct stat dirStat;
        stat(dirPath.string().c_str(), &dirStat);
        get->dir._st_atime = dirStat.st_atime;
        get->dir._st_mtime = dirStat.st_mtime;
        get->dir._st_ctime = dirStat.st_ctime;
        strncpy(get->dir.dirName,
                path(dirPath).filename().string().c_str(),
                CELL_SAVEDATA_DIRNAME_SIZE);
    }
    
    auto sfoPath = dirPath / "PARAM.SFO";
    if (exists(sfoPath)) {
        
    }
    
    get->bind = CELL_SAVEDATA_BINDSTAT_OK;
    // sizes, numbers
    
    result->userdata = userdata;
    set->setParam = setParamVa;
    set->indicator = setIndicatorVa;
    
    emuCallback(funcStat, {resultVa, getVa, setVa}, true);
    if (result->result == CELL_SAVEDATA_CBRESULT_ERR_NODATA ||
        result->result == CELL_SAVEDATA_CBRESULT_ERR_INVALID) {
        return CELL_SAVEDATA_ERROR_CBRESULT;
    }
    
    assert(result->result == CELL_SAVEDATA_CBRESULT_OK_NEXT);
    
    for (auto i = 0u; i < setBuf->fileListMax; ++i) {
        emuCallback(funcFile, {resultVa, getVa, setVa}, true);
    }
    
    return CELL_OK;
}

int32_t cellSaveDataAutoSave2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata) {
    return cellSaveDataAutoSaveLoad(version,
                                    dirName,
                                    errDialog,
                                    setBuf,
                                    funcStat,
                                    funcFile,
                                    container,
                                    userdata,
                                    false);
}

int32_t cellSaveDataAutoLoad2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata) {
    return cellSaveDataAutoSaveLoad(version,
                                    dirName,
                                    errDialog,
                                    setBuf,
                                    funcStat,
                                    funcFile,
                                    container,
                                    userdata,
                                    true);
}
