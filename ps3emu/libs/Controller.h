#pragma once

#include "sys_defs.h"
#include <stdint.h>
#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

#define CELL_PAD_STATUS_DISCONNECTED (0)
#define CELL_PAD_STATUS_CONNECTED (1 << 0)

#define CELL_PAD_CTRL_LEFT      (1 << 7)
#define CELL_PAD_CTRL_DOWN      (1 << 6)
#define CELL_PAD_CTRL_RIGHT     (1 << 5)
#define CELL_PAD_CTRL_UP        (1 << 4)
#define CELL_PAD_CTRL_START     (1 << 3)
#define CELL_PAD_CTRL_R3        (1 << 2)
#define CELL_PAD_CTRL_L3        (1 << 1)
#define CELL_PAD_CTRL_SELECT    (1 << 0)

#define CELL_PAD_CTRL_SQUARE    (1 << 7)
#define CELL_PAD_CTRL_CROSS     (1 << 6)
#define CELL_PAD_CTRL_CIRCLE    (1 << 5)
#define CELL_PAD_CTRL_TRIANGLE  (1 << 4)
#define CELL_PAD_CTRL_R1        (1 << 3)
#define CELL_PAD_CTRL_L1        (1 << 2)
#define CELL_PAD_CTRL_R2        (1 << 1)
#define CELL_PAD_CTRL_L2        (1 << 0)

enum CellPadButtonDataOffset {
    CELL_PAD_BTN_OFFSET_DIGITAL1 = 2,
    CELL_PAD_BTN_OFFSET_DIGITAL2 = 3,
    CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X = 4,
    CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y = 5,
    CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X  = 6,
    CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y  = 7,
    CELL_PAD_BTN_OFFSET_PRESS_RIGHT    = 8,
    CELL_PAD_BTN_OFFSET_PRESS_LEFT     = 9,
    CELL_PAD_BTN_OFFSET_PRESS_UP       = 10,
    CELL_PAD_BTN_OFFSET_PRESS_DOWN     = 11,
    CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE = 12,
    CELL_PAD_BTN_OFFSET_PRESS_CIRCLE   = 13,
    CELL_PAD_BTN_OFFSET_PRESS_CROSS    = 14,
    CELL_PAD_BTN_OFFSET_PRESS_SQUARE   = 15,
    CELL_PAD_BTN_OFFSET_PRESS_L1       = 16,
    CELL_PAD_BTN_OFFSET_PRESS_R1       = 17,
    CELL_PAD_BTN_OFFSET_PRESS_L2       = 18,
    CELL_PAD_BTN_OFFSET_PRESS_R2       = 19,
    CELL_PAD_BTN_OFFSET_SENSOR_X       = 20,
    CELL_PAD_BTN_OFFSET_SENSOR_Y       = 21,
    CELL_PAD_BTN_OFFSET_SENSOR_Z       = 22,
    CELL_PAD_BTN_OFFSET_SENSOR_G       = 23
};

#define CELL_PAD_OK CELL_OK
#define CELL_KB_OK CELL_OK
#define CELL_KB_ERROR_NO_DEVICE 0x80121007
#define CELL_MOUSE_ERROR_NO_DEVICE 0x80121207
#define CELL_MOUSE_OK CELL_OK

#define CELL_PAD_MAX_PORT_NUM (7)

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
int32_t cellKbSetReadMode(uint32_t port_no, uint32_t rmode);
int32_t cellKbRead(uint32_t port_no, uint32_t data);

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
int32_t cellMouseGetData(uint32_t port_no, uint32_t data);
int32_t cellPadGetData(uint32_t port_no, CellPadData *data);
int32_t cellPadSetActDirect(uint32_t port_no, CellPadActParam* param);
