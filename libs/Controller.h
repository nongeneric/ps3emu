#pragma once

#include "sys_defs.h"
#include <stdint.h>
#include <boost/endian/arithmetic.hpp>

using namespace boost::endian;

#define CELL_PAD_OK     CELL_OK
#define CELL_KB_OK     CELL_OK
#define CELL_MOUSE_OK     CELL_OK

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

int32_t cellMouseInit(uint32_t max_connect);
int32_t cellMouseEnd();
int32_t cellMouseGetInfo(CellMouseInfo* info);
int32_t cellPadGetData(uint32_t port_no, CellPadData *data);