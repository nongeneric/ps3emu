#pragma once

#include "../sys.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/enum.h"

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

ENUM(CellAudioOut,
    (CELL_AUDIO_OUT_PRIMARY, 0),
    (CELL_AUDIO_OUT_SECONDARY, 1));

ENUM(CellAudioOutCopyControl,
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_FREE, 0),
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE, 1),
    (CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER, 2));

int32_t cellAudioInit();
int32_t cellAudioPortOpen(const CellAudioPortParam* audioParam, big_uint32_t* portNum);
int32_t cellAudioGetPortConfig(uint32_t portNum, CellAudioPortConfig* portConfig);
int32_t cellAudioPortStart(uint32_t portNum);
int32_t cellAudioSetNotifyEventQueue(sys_ipc_key_t key);
int32_t cellAudioOutSetCopyControl(CellAudioOut audioOut, CellAudioOutCopyControl control);
int32_t cellAudioCreateNotifyEventQueue(sys_event_queue_t *id, sys_ipc_key_t *key);
int32_t cellAudioGetPortBlockTag(uint32_t portNum, uint64_t index, big_uint64_t *frameTag);
int32_t cellAudioGetPortTimestamp(uint32_t portNum, uint64_t frameTag, big_uint64_t *timeStamp);
int32_t cellAudioAddData(uint32_t portNum, uint32_t src, uint32_t samples, float volume);
