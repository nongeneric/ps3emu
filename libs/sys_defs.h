#pragma once

#include <stdint.h>
#include "../ps3emu/constants.h"
#include <boost/endian/arithmetic.hpp>
using namespace boost::endian;

#define CELL_OK 0
#define CELL_STATUS_IS_FAILURE(status) ((status) & 0x80000000)
#define CELL_STATUS_IS_SUCCESS(status) (!((status) & 0x80000000))

typedef big_int64_t cell_time_t;
typedef big_int64_t cell_system_time_t;