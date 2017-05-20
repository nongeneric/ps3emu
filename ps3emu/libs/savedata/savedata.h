#pragma once

#include "ps3emu/libs/sys.h"

struct CellSaveDataSetBuf {
    big_uint32_t dirListMax;
    big_uint32_t fileListMax;
    big_uint32_t reserved[6];
    big_uint32_t bufSize;
    big_uint32_t buf;
};

int32_t cellSaveDataAutoSave2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata);

int32_t cellSaveDataAutoLoad2(uint32_t version,
                              cstring_ptr_t dirName,
                              uint32_t errDialog,
                              const CellSaveDataSetBuf* setBuf,
                              uint32_t funcStat,
                              uint32_t funcFile,
                              uint32_t container,
                              uint32_t userdata);