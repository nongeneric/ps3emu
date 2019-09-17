#pragma once

#include "ps3emu/libs/sys.h"
#include <boost/context/detail/fcontext.hpp>

struct CellSaveDataSetBuf {
    big_uint32_t dirListMax;
    big_uint32_t fileListMax;
    big_uint32_t reserved[6];
    big_uint32_t bufSize;
    big_uint32_t buf;
};

typedef struct {
    big_uint32_t sortType;
    big_uint32_t sortOrder;
    big_uint32_t dirNamePrefix;
    big_uint32_t reserved;
} CellSaveDataSetList;

int32_t cellSaveDataAutoSave2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata,
                              boost::context::continuation* sink);

int32_t cellSaveDataAutoLoad2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata,
                              boost::context::continuation* sink);

emu_void_t cellSaveDataEnableOverlay(int32_t enable);
int32_t cellSaveDataListLoad2(uint32_t version,
                              CellSaveDataSetList* setList,
                              CellSaveDataSetBuf* setBuf,
                              uint32_t funcList,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata);
