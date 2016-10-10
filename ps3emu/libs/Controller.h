#pragma once

#include "sys_defs.h"
#include <stdint.h>
#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

#define CELL_PAD_OK     CELL_OK
#define CELL_KB_OK     CELL_OK
#define CELL_MOUSE_OK     CELL_OK

#define CELL_PAD_MAX_PORT_NUM (7)

enum GlfwDS4Buttons {
    GLFW_DS4_BUTTON_SQUARE = 0,
    GLFW_DS4_BUTTON_CROSS,
    GLFW_DS4_BUTTON_CIRCLE,
    GLFW_DS4_BUTTON_TRIANGLE,
    GLFW_DS4_BUTTON_L1,
    GLFW_DS4_BUTTON_R1,
    GLFW_DS4_BUTTON_L2,
    GLFW_DS4_BUTTON_R2,
    GLFW_DS4_BUTTON_SELECT,
    GLFW_DS4_BUTTON_START,
    GLFW_DS4_BUTTON_L3,
    GLFW_DS4_BUTTON_R3,
    GLFW_DS4_BUTTON_PS,
    GLFW_DS4_BUTTON_TOUCHPAD
};

enum GlfwDS4Axes {
    GLFW_DS4_AXIS_LEFT_X = 0,
    GLFW_DS4_AXIS_LEFT_Y,
    GLFW_DS4_AXIS_RIGHT_X,
    GLFW_DS4_AXIS_L2,
    GLFW_DS4_AXIS_R2,
    GLFW_DS4_AXIS_RIGHT_Y,
    GLFW_DS4_AXIS_LEFT_RIGHT,
    GLFW_DS4_AXIS_DOWN_UP
};

typedef struct CellPadInfo2 {
    big_uint32_t max_connect;
    big_uint32_t now_connect;
    big_uint32_t system_info;
    big_uint32_t port_status[CELL_PAD_MAX_PORT_NUM];
    big_uint32_t port_setting[CELL_PAD_MAX_PORT_NUM];
    big_uint32_t device_capability[CELL_PAD_MAX_PORT_NUM];
    big_uint32_t device_type[CELL_PAD_MAX_PORT_NUM];
} CellPadInfo2;

int32_t cellPadInit(uint32_t max_connect);
int32_t cellPadEnd();
int32_t cellPadGetInfo2(CellPadInfo2* info);
int32_t cellPadClearBuf(uint32_t port_no);
int32_t cellPadSetPortSetting(uint32_t port_no, uint32_t port_setting);

#define CELL_KB_MAX_KEYBOARDS  127
typedef struct CellKbInfo{
    big_uint32_t max_connect;
    big_uint32_t now_connect;
    big_uint32_t info;
    big_uint8_t status[CELL_KB_MAX_KEYBOARDS];
} CellKbInfo;

int32_t cellKbInit(uint32_t max_connect);
int32_t cellKbEnd();
int32_t cellKbSetCodeType(uint32_t port_no, uint32_t type);
int32_t cellKbGetInfo(CellKbInfo* info);

#define CELL_MAX_MICE 127
typedef struct CellMouseInfo{
    big_uint32_t max_connect;
    big_uint32_t now_connect;
    big_uint32_t info;
    big_uint16_t vendor_id[CELL_MAX_MICE];
    big_uint16_t product_id[CELL_MAX_MICE];
    big_uint8_t status[CELL_MAX_MICE];
} CellMouseInfo;

#define CELL_PAD_MAX_CODES (64)
typedef struct CellPadData{
  big_int32_t len;
  big_uint16_t button[CELL_PAD_MAX_CODES];
} CellPadData;

#define CELL_PAD_ACTUATOR_MAX   (2)
typedef struct CellPadActParam
{
    uint8_t motor[CELL_PAD_ACTUATOR_MAX];
    uint8_t reserved[6];
} CellPadActParam;

int32_t cellMouseInit(uint32_t max_connect);
int32_t cellMouseEnd();
int32_t cellMouseGetInfo(CellMouseInfo* info);
int32_t cellPadGetData(uint32_t port_no, CellPadData *data);
int32_t cellPadSetActDirect(uint32_t port_no, CellPadActParam* param);
