#pragma once

#include "../sys.h"

typedef struct {
    big_uint64_t nChannel;
    big_uint64_t nBlock;
    big_uint64_t attr;
    big_uint32_t float_level;
} CellAudioPortParam;

typedef struct {
    sys_addr_t readIndexAddr;
    big_uint32_t status;
    big_uint64_t nChannel;
    big_uint64_t nBlock;
    big_uint32_t portSize;
    sys_addr_t portAddr;
} CellAudioPortConfig;

int32_t cellAudioInit();
int32_t cellAudioPortOpen(const CellAudioPortParam* audioParam, big_uint32_t* portNum);
int32_t cellAudioGetPortConfig(uint32_t portNum, CellAudioPortConfig* portConfig);
int32_t cellAudioPortStart(uint32_t portNum);
int32_t cellAudioSetNotifyEventQueue(sys_ipc_key_t key);
