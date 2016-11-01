#pragma once

#include "sys.h"
#include "ps3emu/enum.h"

typedef union CellSpursTaskLsPattern {
    big_uint32_t u32[4];
    big_uint64_t u64[2];
} CellSpursTaskLsPattern;

typedef struct CellSpursTaskAttribute2 {
    big_uint32_t revision;
    big_uint32_t sizeContext;
    big_uint64_t eaContext;
    CellSpursTaskLsPattern lsPattern;
    big_uint32_t name;
} CellSpursTaskAttribute2;

typedef struct CellSpursTaskAttribute {
    big_uint32_t revision;
    big_uint32_t sizeContext;
    big_uint64_t eaContext;
} CellSpursTaskAttribute;

typedef struct CellSpursTasksetAttribute2 {
    big_uint32_t revision;
    big_uint32_t nameVa;
    big_uint64_t argTaskset;
    uint8_t priority[8];
    big_uint32_t maxContention;
    big_int32_t enableClearLs;
    big_uint32_t taskNameBuffer;
} CellSpursTasksetAttribute2;

typedef union CellSpursTaskArgument {
    big_uint32_t u32[4];
    big_uint64_t u64[2];
} CellSpursTaskArgument;

ENUM(CellSpursEventFlagWaitMode,
     (CELL_SPURS_EVENT_FLAG_OR, 0),
     (CELL_SPURS_EVENT_FLAG_AND, 1))

ENUM(CellSpursEventFlagClearMode,
     (CELL_SPURS_EVENT_FLAG_CLEAR_AUTO, 0),
     (CELL_SPURS_EVENT_FLAG_CLEAR_MANUAL, 1))

ENUM(CellSpursEventFlagDirection,
     (CELL_SPURS_EVENT_FLAG_SPU2SPU, 0),
     (CELL_SPURS_EVENT_FLAG_SPU2PPU, 1),
     (CELL_SPURS_EVENT_FLAG_PPU2SPU, 2),
     (CELL_SPURS_EVENT_FLAG_ANY2ANY, 3))

ENUM(CellSpursQueueDirection,
     (CELL_SPURS_QUEUE_SPU2SPU, 0),
     (CELL_SPURS_QUEUE_SPU2PPU, 1),
     (CELL_SPURS_QUEUE_PPU2SPU, 2))

emu_void_t _cellSpursTaskAttribute2Initialize_proxy(const CellSpursTaskAttribute2 *pAttr);
emu_void_t cellSpursCreateTaskset2_proxy(uint32_t spurs,
                                         uint32_t taskset2,
                                         const CellSpursTasksetAttribute2* attr);
emu_void_t cellSpursCreateTask2_proxy(uint32_t taskset,
                                      uint32_t id,
                                      uint32_t eaElf,
                                      const CellSpursTaskArgument* arg,
                                      const CellSpursTaskAttribute2* attr);
emu_void_t cellSpursCreateTaskWithAttribute_proxy(uint32_t taskset,
                                                  uint32_t id,
                                                  const CellSpursTaskAttribute*);
emu_void_t cellSpursEventFlagAttachLv2EventQueue_proxy(uint32_t ev);
emu_void_t _cellSpursEventFlagInitialize_proxy(
    uint32_t unk,
    uint32_t taskset,
    uint32_t ev,
    CellSpursEventFlagClearMode clearMode,
    CellSpursEventFlagDirection direction);
emu_void_t cellSyncQueueInitialize_proxy(uint32_t obj,
                                         uint32_t ptr_buffer,
                                         uint32_t buffer_size,
                                         uint32_t depth);
emu_void_t _cellSpursQueueInitialize_proxy(uint32_t taskset,
                                           uint32_t ea,
                                           uint32_t buffer,
                                           uint32_t size,
                                           uint32_t depth,
                                           CellSpursQueueDirection direction);
emu_void_t cellSpursEventFlagWait_proxy(uint32_t flag,
                                        const uint16_t* bits,
                                        CellSpursEventFlagWaitMode mode);
