#pragma once

#include "../sys_defs.h"
#include <boost/endian/arithmetic.hpp>

typedef struct CellVideoOutDisplayMode {
    uint8_t resolutionId;
    uint8_t scanMode;
    uint8_t conversion;
    uint8_t aspect;
    uint8_t reserved[2];
    boost::endian::big_uint16_t refreshRates;
} CellVideoOutDisplayMode;

typedef struct CellVideoOutState {
    uint8_t state;
    uint8_t colorSpace;
    uint8_t reserved[6];
    CellVideoOutDisplayMode displayMode;
} CellVideoOutState;

typedef struct CellVideoOutResolution {
    boost::endian::big_uint16_t width;
    boost::endian::big_uint16_t height;
} CellVideoOutResolution;

typedef enum CellVideoOut {
    CELL_VIDEO_OUT_PRIMARY,
    CELL_VIDEO_OUT_SECONDARY
} CellVideoOut;

typedef enum CellVideoOutColorSpace {
    CELL_VIDEO_OUT_COLOR_SPACE_RGB = 0x01,
    CELL_VIDEO_OUT_COLOR_SPACE_YUV = 0x02,
    CELL_VIDEO_OUT_COLOR_SPACE_XVYCC = 0x04
} CellVideoOutColorSpace;

typedef enum CellVideoOutDisplayAspect {
    CELL_VIDEO_OUT_ASPECT_AUTO,
    CELL_VIDEO_OUT_ASPECT_4_3,
    CELL_VIDEO_OUT_ASPECT_16_9
} CellVideoOutDisplayAspect;

typedef enum CellVideoOutDisplayConversion {
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE = 0x00,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WXGA = 0x01,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_SXGA = 0x02,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_WUXGA = 0x03,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_1080 = 0x05,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_REMOTEPLAY = 0x10,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_3D_FRAME_PACKING = 0x80,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_DUALVIEW_FRAME_PACKING = 0x81,
    CELL_VIDEO_OUT_DISPLAY_CONVERSION_TO_720_SIMULVIEW_FRAME_PACKING = 0x81
} CellVideoOutDisplayConversion;

typedef enum CellVideoOutRefreshRate {
    CELL_VIDEO_OUT_REFRESH_RATE_AUTO = 0x0000,
    CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ = 0x0001,
    CELL_VIDEO_OUT_REFRESH_RATE_50HZ = 0x0002,
    CELL_VIDEO_OUT_REFRESH_RATE_60HZ = 0x0004,
    CELL_VIDEO_OUT_REFRESH_RATE_30HZ = 0x0008
} CellVideoOutRefreshRate;

typedef enum CellVideoOutResolutionId {
    CELL_VIDEO_OUT_RESOLUTION_UNDEFINED = 0,
    CELL_VIDEO_OUT_RESOLUTION_1080 = 1,
    CELL_VIDEO_OUT_RESOLUTION_720 = 2,
    CELL_VIDEO_OUT_RESOLUTION_480 = 4,
    CELL_VIDEO_OUT_RESOLUTION_576 = 5,
    CELL_VIDEO_OUT_RESOLUTION_1600x1080 = 10,
    CELL_VIDEO_OUT_RESOLUTION_1440x1080 = 11,
    CELL_VIDEO_OUT_RESOLUTION_1280x1080 = 12,
    CELL_VIDEO_OUT_RESOLUTION_960x1080 = 13
    ,
    CELL_VIDEO_OUT_RESOLUTION_720_3D_FRAME_PACKING = 0x81,
    CELL_VIDEO_OUT_RESOLUTION_1024x720_3D_FRAME_PACKING = 0x88,
    CELL_VIDEO_OUT_RESOLUTION_960x720_3D_FRAME_PACKING = 0x89,
    CELL_VIDEO_OUT_RESOLUTION_800x720_3D_FRAME_PACKING = 0x8a,
    CELL_VIDEO_OUT_RESOLUTION_640x720_3D_FRAME_PACKING = 0x8b,
    CELL_VIDEO_OUT_RESOLUTION_720_DUALVIEW_FRAME_PACKING = 0x91,
    CELL_VIDEO_OUT_RESOLUTION_720_SIMULVIEW_FRAME_PACKING = 0x91,
    CELL_VIDEO_OUT_RESOLUTION_1024x720_DUALVIEW_FRAME_PACKING = 0x98,
    CELL_VIDEO_OUT_RESOLUTION_1024x720_SIMULVIEW_FRAME_PACKING = 0x98,
    CELL_VIDEO_OUT_RESOLUTION_960x720_DUALVIEW_FRAME_PACKING = 0x99,
    CELL_VIDEO_OUT_RESOLUTION_960x720_SIMULVIEW_FRAME_PACKING = 0x99,
    CELL_VIDEO_OUT_RESOLUTION_800x720_DUALVIEW_FRAME_PACKING = 0x9a,
    CELL_VIDEO_OUT_RESOLUTION_800x720_SIMULVIEW_FRAME_PACKING = 0x9a,
    CELL_VIDEO_OUT_RESOLUTION_640x720_DUALVIEW_FRAME_PACKING = 0x9b,
    CELL_VIDEO_OUT_RESOLUTION_640x720_SIMULVIEW_FRAME_PACKING = 0x9b
} CellVideoOutResolutionId;

typedef enum CellVideoOutScanMode {
    CELL_VIDEO_OUT_SCAN_MODE_INTERLACE,
    CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE
} CellVideoOutScanMode;

typedef enum CellVideoOutOutputState {
    CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED,
    CELL_VIDEO_OUT_OUTPUT_STATE_DISABLED,
    CELL_VIDEO_OUT_OUTPUT_STATE_PREPARING
} CellVideoOutOutputState;

typedef struct CellVideoOutOption {
    boost::endian::big_uint32_t reserved;
} CellVideoOutOption;

typedef struct CellVideoOutConfiguration {
    uint8_t resolutionId;
    uint8_t format;
    uint8_t aspect;
    uint8_t reserved[9];
    boost::endian::big_uint32_t pitch;
} CellVideoOutConfiguration;

typedef enum CellVideoOutBufferColorFormat {
    CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8,
    CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8,
    CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT
} CellVideoOutBufferColorFormat;

int cellVideoOutGetState(uint32_t videoOut, uint32_t deviceIndex, CellVideoOutState* state);
int cellVideoOutGetResolution(uint32_t resolutionId, CellVideoOutResolution* resolution);
int cellVideoOutConfigure(uint32_t videoOut, 
                          CellVideoOutConfiguration* config,
                          CellVideoOutOption* option, 
                          uint32_t waitForEvent);
