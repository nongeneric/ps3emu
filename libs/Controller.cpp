#include "Controller.h"

int32_t cellPadInit(uint32_t max_connect) {
    return CELL_PAD_OK;
}

int32_t cellKbInit(uint32_t max_connect) {
    return CELL_KB_OK;
}

int32_t cellKbSetCodeType(uint32_t port_no, uint32_t type) {
    return CELL_KB_OK;
}

int32_t cellMouseInit(uint32_t max_connect) {
    return CELL_MOUSE_OK;
}

#define CELL_PAD_STATUS_DISCONNECTED    (0) 

int32_t cellPadGetInfo2(CellPadInfo2* info) {
    info->max_connect = 1;
    info->now_connect = 0;
    for (int i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i) {
        info->port_status[i] = CELL_PAD_STATUS_DISCONNECTED;
    }
    return CELL_PAD_OK;
}

#define CELL_KB_STATUS_CONNECTED       (1)
#define CELL_KB_STATUS_DISCONNECTED    (0)

int32_t cellKbGetInfo(CellKbInfo* info) {
    info->max_connect = 1;
    info->now_connect = 0;
    for (int i = 0; i < CELL_KB_MAX_KEYBOARDS; ++i) {
        info->status[i] = CELL_KB_STATUS_DISCONNECTED;
    }
    return CELL_KB_OK;
}

int32_t cellMouseGetInfo(CellMouseInfo* info) {
    info->max_connect = 1;
    info->max_connect = 0;
    for (int i = 0; i < CELL_MAX_MICE; ++i) {
        info->status[i] = 0;
    }
    return CELL_MOUSE_OK;
}

int32_t cellPadEnd() {
    return CELL_OK;
}

int32_t cellPadGetData(uint32_t port_no, CellPadData* data) {
    data->len = 0;
    return CELL_OK;
}

int32_t cellKbEnd() {
    return CELL_OK;
}

int32_t cellMouseEnd() {
    return CELL_OK;
}
