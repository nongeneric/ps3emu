#pragma once

#include "sys.h"

#define CELL_PNGDEC_ERROR_HEADER 0x80611201
#define CELL_PNGDEC_ERROR_STREAM_FORMAT 0x80611202
#define CELL_PNGDEC_ERROR_ARG 0x80611203
#define CELL_PNGDEC_ERROR_SEQ 0x80611204
#define CELL_PNGDEC_ERROR_BUSY 0x80611205
#define CELL_PNGDEC_ERROR_FATAL 0x80611206
#define CELL_PNGDEC_ERROR_OPEN_FILE 0x80611207
#define CELL_PNGDEC_ERROR_SPU_UNSUPPORT 0x80611208
#define CELL_PNGDEC_ERROR_SPU_ERROR 0x80611209
#define CELL_PNGDEC_ERROR_CB_PARAM 0x8061120a

typedef big_uint32_t CellPngDecMainHandle;
typedef big_uint32_t CellPngDecSubHandle;

typedef enum {
    CELL_PNGDEC_SPU_THREAD_DISABLE = 0,
    CELL_PNGDEC_SPU_THREAD_ENABLE = 1
} CellPngDecSpuThreadEna;

typedef enum {
    CELL_PNGDEC_DEC_STATUS_FINISH = 0,
    CELL_PNGDEC_DEC_STATUS_STOP = 1
} CellPngDecDecodeStatus;

typedef struct CellPngDecThreadInParam {
    big_uint32_t spuThreadEnable; // CellPngDecSpuThreadEna
    big_uint32_t ppuThreadPriority;
    big_uint32_t spuThreadPriority;
    big_uint32_t cbCtrlMallocFunc; // CellPngDecCbControlMalloc
    big_uint32_t cbCtrlMallocArg;
    big_uint32_t cbCtrlFreeFunc; // CellPngDecCbControlFree
    big_uint32_t cbCtrlFreeArg;
} CellPngDecThreadInParam;

typedef struct CellPngDecThreadOutParam {
    big_uint32_t pngCodecVersion;
} CellPngDecThreadOutParam;

typedef struct CellPngDecDataCtrlParam {
    big_uint64_t outputBytesPerLine;
} CellPngDecDataCtrlParam;

typedef struct CellPngDecDataOutInfo {
    big_uint32_t chunkInformation;
    big_uint32_t numText;
    big_uint32_t numUnknownChunk;
    big_uint32_t status; // CellPngDecDecodeStatus
} CellPngDecDataOutInfo;

typedef enum {
    CELL_PNGDEC_GRAYSCALE = 1,
    CELL_PNGDEC_RGB = 2,
    CELL_PNGDEC_PALETTE = 4,
    CELL_PNGDEC_GRAYSCALE_ALPHA = 9,
    CELL_PNGDEC_RGBA = 10,
    CELL_PNGDEC_ARGB = 20
} CellPngDecColorSpace;

typedef enum {
    CELL_PNGDEC_NO_INTERLACE = 0,
    CELL_PNGDEC_ADAM7_INTERLACE = 1
} CellPngDecInterlaceMode;

typedef struct CellPngDecInfo {
    big_uint32_t imageWidth;
    big_uint32_t imageHeight;
    big_uint32_t numComponents;
    big_uint32_t colorSpace; // CellPngDecColorSpace
    big_uint32_t bitDepth;
    big_uint32_t interlaceMethod; // CellPngDecInterlaceMode
    big_uint32_t chunkInformation;
} CellPngDecInfo;

typedef enum { CELL_PNGDEC_FILE = 0, CELL_PNGDEC_BUFFER = 1 } CellPngDecStreamSrcSel;

typedef struct CellPngDecSrc {
    big_uint32_t srcSelect; // CellPngDecStreamSrcSel
    big_uint32_t fileName; // char*
    big_int64_t fileOffset;
    big_uint32_t fileSize;
    big_uint32_t streamPtr;
    big_uint32_t streamSize;
    big_uint32_t spuThreadEnable; // CellPngDecSpuThreadEna
} CellPngDecSrc;

typedef struct CellPngDecOpnInfo {
    big_uint32_t initSpaceAllocated;
} CellPngDecOpnInfo;

typedef enum { CELL_PNGDEC_CONTINUE = 0, CELL_PNGDEC_STOP = 1 } CellPngDecCommand;

typedef enum {
    CELL_PNGDEC_TOP_TO_BOTTOM = 0,
    CELL_PNGDEC_BOTTOM_TO_TOP = 1
} CellPngDecOutputMode;

typedef enum {
    CELL_PNGDEC_1BYTE_PER_NPIXEL = 0,
    CELL_PNGDEC_1BYTE_PER_1PIXEL = 1
} CellPngDecPackFlag;

typedef enum {
    CELL_PNGDEC_STREAM_ALPHA = 0,
    CELL_PNGDEC_FIX_ALPHA = 1
} CellPngDecAlphaSelect;

typedef struct CellPngDecInParam {
    big_uint32_t commandPtr;      // CellPngDecCommand
    big_uint32_t outputMode;       // CellPngDecOutputMode
    big_uint32_t outputColorSpace; // CellPngDecColorSpace
    big_uint32_t outputBitDepth;
    big_uint32_t outputPackFlag;    // CellPngDecPackFlag
    big_uint32_t outputAlphaSelect; // CellPngDecAlphaSelect
    big_uint32_t outputColorAlpha;
} CellPngDecInParam;

typedef struct CellPngDecOutParam {
    big_uint64_t outputWidthByte;
    big_uint32_t outputWidth;
    big_uint32_t outputHeight;
    big_uint32_t outputComponents;
    big_uint32_t outputBitDepth;
    big_uint32_t outputMode;       // CellPngDecOutputMode
    big_uint32_t outputColorSpace; // CellPngDecColorSpace
    big_uint32_t useMemorySpace;
} CellPngDecOutParam;

class Process;

int32_t cellPngDecCreate(CellPngDecMainHandle* mainHandle,
                         const CellPngDecThreadInParam* threadInParam,
                         CellPngDecThreadOutParam* threadOutParam);

int32_t cellPngDecDecodeData(CellPngDecMainHandle mainHandle,
                             CellPngDecSubHandle subHandle,
                             ps3_uintptr_t data,
                             const CellPngDecDataCtrlParam* dataCtrlParam,
                             CellPngDecDataOutInfo* dataOutInfo,
                             MainMemory* mm);

int32_t cellPngDecClose(CellPngDecMainHandle mainHandle,
                        CellPngDecSubHandle subHandle);

int32_t cellPngDecDestroy(CellPngDecMainHandle mainHandle);

int32_t cellPngDecReadHeader(CellPngDecMainHandle mainHandle,
                             CellPngDecSubHandle subHandle,
                             CellPngDecInfo* info);

int32_t cellPngDecOpen(CellPngDecMainHandle mainHandle,
                       CellPngDecSubHandle* subHandle,
                       const CellPngDecSrc* src,
                       CellPngDecOpnInfo* openInfo,
                       Process* proc);

int32_t cellPngDecSetParameter(CellPngDecMainHandle mainHandle,
                               CellPngDecSubHandle subHandle,
                               const CellPngDecInParam* inParam,
                               CellPngDecOutParam* outParam);
