#include "ps3emu/libs/libsysutil.h"
#include "savedata.h"
#include "SfoDb.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/ContentManager.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/fileutils.h"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
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
    big_uint64_t _st_size;
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
    big_uint32_t fileList; // CellSaveDataFileStat*
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

struct CellSaveDataFileGet {
    big_uint32_t excSize;
    char reserved[64];
};

struct CellSaveDataFileSet {
    big_uint32_t fileOperation;
    big_uint32_t reserved;
    big_uint32_t fileType;
    uint8_t secureFileId[CELL_SAVEDATA_SECUREFILEID_SIZE];
    big_uint32_t fileName; // char*
    big_uint32_t fileOffset;
    big_uint32_t fileSize;
    big_uint32_t fileBufSize;
    big_uint32_t fileBuf;
};

struct CellSaveDataDoneGet {
    big_int32_t excResult;
    char dirName[CELL_SAVEDATA_DIRNAME_SIZE];
    big_int32_t sizeKB;
    big_int32_t hddFreeSizeKB;
    char reserved[64];
};

enum CellSaveDataFileType {
    CELL_SAVEDATA_FILETYPE_SECUREFILE = 0,
    CELL_SAVEDATA_FILETYPE_NORMALFILE,
    CELL_SAVEDATA_FILETYPE_CONTENT_ICON0,
    CELL_SAVEDATA_FILETYPE_CONTENT_ICON1,
    CELL_SAVEDATA_FILETYPE_CONTENT_PIC1,
    CELL_SAVEDATA_FILETYPE_CONTENT_SND0
};

#define CELL_SAVEDATA_BINDSTAT_OK 0
#define CELL_SAVEDATA_ERROR_CBRESULT (0x8002b401)
#define CELL_SAVEDATA_CBRESULT_OK_NEXT 0
#define CELL_SAVEDATA_CBRESULT_ERR_NOSPACE (-1)
#define CELL_SAVEDATA_CBRESULT_ERR_FAILURE (-2)
#define CELL_SAVEDATA_CBRESULT_ERR_BROKEN (-3)
#define CELL_SAVEDATA_CBRESULT_ERR_NODATA (-4)
#define CELL_SAVEDATA_CBRESULT_ERR_INVALID (-5)

enum CellSaveDataFileOperation {
    CELL_SAVEDATA_FILEOP_READ = 0,
    CELL_SAVEDATA_FILEOP_WRITE,
    CELL_SAVEDATA_FILEOP_DELETE,
    CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC
};

std::string secureFileSuffix = "_secure_file_suffix_";
std::string paramSfoName = "PARAM.SFO.db";

std::string getFileName(CellSaveDataFileSet* set) {
    switch (set->fileType) {
        case CELL_SAVEDATA_FILETYPE_CONTENT_ICON0:
            return "ICON0.PNG";
        case CELL_SAVEDATA_FILETYPE_CONTENT_ICON1:
            return "ICON1.PAM";
        case CELL_SAVEDATA_FILETYPE_CONTENT_PIC1:
            return "PIC1.PNG";
        case CELL_SAVEDATA_FILETYPE_CONTENT_SND0:
            return "SND0.AT3";
        default: {
            std::string res;
            readString(g_state.mm, set->fileName, res);
            return res;
        }
    }
}

std::tuple<std::string, unsigned> parseFileName(std::string path) {
    auto name = boost::filesystem::path(path).filename().string();
    if (name == "ICON0.PNG")
        return {name, CELL_SAVEDATA_FILETYPE_CONTENT_ICON0};
    if (name == "ICON1.PAM")
        return {name, CELL_SAVEDATA_FILETYPE_CONTENT_ICON1};
    if (name == "PIC1.PNG")
        return {name, CELL_SAVEDATA_FILETYPE_CONTENT_PIC1};
    if (name == "SND0.AT3")
        return {name, CELL_SAVEDATA_FILETYPE_CONTENT_SND0};
    if (boost::algorithm::ends_with(name, secureFileSuffix))
        return {name.substr(0, name.size() - secureFileSuffix.size()), CELL_SAVEDATA_FILETYPE_SECUREFILE};
    return {name, CELL_SAVEDATA_FILETYPE_NORMALFILE};
}

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
        
    uint32_t resultVa, getVa, setVa, setParamVa, setIndicatorVa, fileGetVa, fileSetVa, fileListVa;
    auto result = g_state.memalloc->internalAlloc<4, CellSaveDataCBResult>(&resultVa);
    auto get = g_state.memalloc->internalAlloc<4, CellSaveDataStatGet>(&getVa);
    auto set = g_state.memalloc->internalAlloc<4, CellSaveDataStatSet>(&setVa);
    g_state.memalloc->internalAlloc<4, CellSaveDataSystemFileParam>(&setParamVa);
    g_state.memalloc->internalAlloc<4, CellSaveDataAutoIndicator>(&setIndicatorVa);
    auto fileGet = g_state.memalloc->internalAlloc<4, CellSaveDataFileGet>(&fileGetVa);
    auto fileSet = g_state.memalloc->internalAlloc<4, CellSaveDataFileSet>(&fileSetVa);
    
    path rootPath =
        g_state.content->toHost(g_state.content->contentDir() + "/SAVEDATA/AUTO/");
    auto dirPath = rootPath / dirName.str;
    auto isNew = !exists(dirPath);
    create_directories(dirPath);
    
    get->hddFreeSizeKB = 100ul << 20ul;
    get->isNewData = isNew;
    
    strncpy(get->dir.dirName,
            dirName.str.c_str(),
            CELL_SAVEDATA_DIRNAME_SIZE);
    
    if (!isNew) {
        struct stat dirStat;
        stat(dirPath.string().c_str(), &dirStat);
        get->dir._st_atime = dirStat.st_atime;
        get->dir._st_mtime = dirStat.st_mtime;
        get->dir._st_ctime = dirStat.st_ctime;
    }

    auto sfoPath = dirPath / paramSfoName;
    SfoDb sfo;
    sfo.open(sfoPath.string());
    auto title = sfo.findKey("TITLE");
    auto subtitle = sfo.findKey("SUB_TITLE");
    auto detail = sfo.findKey("DETAIL");
    auto attribute = sfo.findKey("ATTRIBUTE");
    auto listParam = sfo.findKey("LIST_PARAM");
    if (title && subtitle && attribute && listParam) {
        strncpy(get->getParam.title,
                std::get<std::string>(*title).c_str(),
                CELL_SAVEDATA_SYSP_TITLE_SIZE);
        strncpy(get->getParam.detail,
                std::get<std::string>(*detail).c_str(),
                CELL_SAVEDATA_SYSP_DETAIL_SIZE);
        strncpy(get->getParam.subTitle,
                std::get<std::string>(*subtitle).c_str(),
                CELL_SAVEDATA_SYSP_SUBTITLE_SIZE);
        strncpy(get->getParam.listParam,
                std::get<std::string>(*listParam).c_str(),
                CELL_SAVEDATA_SYSP_LPARAM_SIZE);
        get->getParam.attribute = std::get<uint32_t>(*attribute);
    }
    
    get->bind = CELL_SAVEDATA_BINDSTAT_OK;
    // sizes, numbers
    
    result->userdata = userdata;
    set->setParam = setParamVa;
    set->indicator = setIndicatorVa;
    
    auto files = get_files(dirPath.string(), "(?!" + paramSfoName + ").*", false);
    std::sort(begin(files), end(files), [&](auto& a, auto& b) {
        auto aname = boost::filesystem::path(a).filename().string();
        auto bname = boost::filesystem::path(b).filename().string();
        return aname < bname;
    });
    auto fileList = (CellSaveDataFileStat*)g_state.memalloc->allocInternalMemory(
        &fileListVa,
        sizeof(CellSaveDataFileStat) * files.size(),
        4);
    get->fileListNum = files.size();
    get->fileNum = files.size();
    get->fileList = fileListVa;
    get->sizeKB = 0;
    if (isLoad) {
        int i = 0;
        for (auto& file : files) {
            auto [name, type] = parseFileName(files[i]);
            fileList[i].fileType = type;
            strncpy(fileList[i].fileName,
                name.c_str(),
                CELL_SAVEDATA_FILENAME_SIZE);
            struct stat fileStat;
            stat(file.c_str(), &fileStat);
            get->sizeKB += fileStat.st_size;
            fileList[i]._st_size = fileStat.st_size;
            fileList[i]._st_atime = fileStat.st_atime;
            fileList[i]._st_mtime = fileStat.st_mtime;
            fileList[i]._st_ctime = fileStat.st_ctime;
            i++;
        }
        get->sizeKB /= 1024;
    }
    
    emuCallback(funcStat, {resultVa, getVa, setVa}, true);
    if (result->result == CELL_SAVEDATA_CBRESULT_ERR_NODATA ||
        result->result == CELL_SAVEDATA_CBRESULT_ERR_INVALID) {
        return CELL_SAVEDATA_ERROR_CBRESULT;
    }
    
    if (set->setParam) {
        CellSaveDataSystemFileParam param;
        g_state.mm->readMemory(set->setParam, &param, sizeof(param));
        sfo.setValue("TITLE", param.title);
        sfo.setValue("SUB_TITLE", param.subTitle);
        sfo.setValue("DETAIL", param.detail);
        sfo.setValue("LIST_PARAM", param.listParam);
        sfo.setValue("ATTRIBUTE", param.attribute);
    }
    
    assert(result->result == CELL_SAVEDATA_CBRESULT_OK_NEXT);
    
    for (;;) {
        emuCallback(funcFile, {resultVa, fileGetVa, fileSetVa}, true);
        if (result->result != CELL_SAVEDATA_CBRESULT_OK_NEXT)
            break;
        
        if (fileSet->fileOperation == CELL_SAVEDATA_FILEOP_READ) {
            auto file = std::find_if(begin(files), end(files), [&](auto& file) {
                return std::get<0>(parseFileName(file)) == getFileName(fileSet);
            });
            if (file == end(files)) {
                WARNING(libs) << ssnprintf("a save file not found");
                break;
            }
            auto body = read_all_bytes(*file);
            auto toWrite = std::min<size_t>(fileSet->fileBufSize, body.size() - fileSet->fileOffset);
            g_state.mm->writeMemory(fileSet->fileBuf, &body[fileSet->fileOffset], toWrite);
            fileGet->excSize = toWrite;
        } else if (fileSet->fileOperation == CELL_SAVEDATA_FILEOP_WRITE) {
            auto fileName = getFileName(fileSet);
            if (fileSet->fileType == CELL_SAVEDATA_FILETYPE_SECUREFILE) {
                fileName += secureFileSuffix;
            }
            auto fileSize = std::min(fileSet->fileBufSize, fileSet->fileSize);
            std::vector<uint8_t> body(fileSize);
            g_state.mm->readMemory(fileSet->fileBuf, &body[0], fileSize);
            write_all_bytes(&body[0], body.size(), (dirPath / fileName).string());
            fileGet->excSize = body.size();
        } else if (fileSet->fileOperation == CELL_SAVEDATA_FILEOP_DELETE) {
            auto file = std::find_if(begin(files), end(files), [&](auto& file) {
                return std::get<0>(parseFileName(file)) == getFileName(fileSet);
            });
            if (file == end(files)) {
                WARNING(libs) << ssnprintf("a save file not found");
                break;
            }
            remove(*file);
        } else if (fileSet->fileOperation == CELL_SAVEDATA_FILEOP_WRITE_NOTRUNC) {
            assert(false);
        }
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

emu_void_t cellSaveDataEnableOverlay(int32_t enable) {
    WARNING(libs) << "cellSaveDataEnableOverlay not implemented";
    return emu_void;
}

int32_t cellSaveDataListLoad2(uint32_t version,
                              CellSaveDataSetList* setList,
                              CellSaveDataSetBuf* setBuf,
                              uint32_t funcList,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata) {
    return -1;
}
