#include "sysutil_sysparam.h"
#include "ps3emu/log.h"
#include "assert.h"

int cellVideoOutGetState(uint32_t videoOut, uint32_t deviceIndex, CellVideoOutState* state) {
    INFO(libs) << __FUNCTION__;
    assert(videoOut == CELL_VIDEO_OUT_PRIMARY);
    assert(deviceIndex == 0);
    state->colorSpace = CELL_VIDEO_OUT_COLOR_SPACE_RGB;
    state->displayMode.aspect = CELL_VIDEO_OUT_ASPECT_16_9;
    state->displayMode.conversion = CELL_VIDEO_OUT_DISPLAY_CONVERSION_NONE;
    state->displayMode.refreshRates = CELL_VIDEO_OUT_REFRESH_RATE_60HZ;
    state->displayMode.resolutionId = CELL_VIDEO_OUT_RESOLUTION_720;
    state->displayMode.scanMode = CELL_VIDEO_OUT_SCAN_MODE_PROGRESSIVE;
    state->state = CELL_VIDEO_OUT_OUTPUT_STATE_ENABLED;
    return CELL_OK;
}

int cellVideoOutGetResolution(uint32_t resolutionId, CellVideoOutResolution* resolution) {
    INFO(libs) << __FUNCTION__;
    resolution->width = 1280;
    resolution->height = 720;
    return CELL_OK;
}

int cellVideoOutConfigure(uint32_t videoOut,
                          CellVideoOutConfiguration* config,
                          CellVideoOutOption* option,
                          uint32_t waitForEvent)
{
    INFO(libs) << __FUNCTION__;
    assert(videoOut == CELL_VIDEO_OUT_PRIMARY);
    assert(option == NULL);
    return CELL_OK;
}
