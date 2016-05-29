#include "libaudio.h"

#define CELL_AUDIO_PORT_2CH 2
#define CELL_AUDIO_PORT_8CH 8

#define CELL_AUDIO_BLOCK_8 8
#define CELL_AUDIO_BLOCK_16 16
#define CELL_AUDIO_BLOCK_32 32

#define CELL_AUDIO_STATUS_CLOSE 0x1010
#define CELL_AUDIO_STATUS_READY 1
#define CELL_AUDIO_STATUS_RUN 2

int32_t cellAudioInit() {
    return CELL_OK;
}

int32_t cellAudioPortOpen(const CellAudioPortParam* audioParam,
                          big_uint32_t* portNum) {
    assert(audioParam->nChannel == CELL_AUDIO_PORT_2CH ||
           audioParam->nChannel == CELL_AUDIO_PORT_8CH);
    assert(audioParam->nBlock == CELL_AUDIO_BLOCK_8 ||
           audioParam->nBlock == CELL_AUDIO_BLOCK_16);
    static int portnums = 0;
    *portNum = portnums++;
    //union_cast<uint32_t, float>(audioParam->float_level);
    return CELL_OK;
}

int32_t cellAudioGetPortConfig(uint32_t portNum, CellAudioPortConfig* portConfig) {
    assert(portNum == 0);
    portConfig->readIndexAddr = 0;
    portConfig->status = CELL_AUDIO_STATUS_CLOSE;
    portConfig->nChannel = 2;
    portConfig->nBlock = 100;
    portConfig->portSize = 100;
    portConfig->portAddr = 0;
    return CELL_OK;
}

int32_t cellAudioPortStart(uint32_t portNum) {
    return CELL_OK;
}

int32_t cellAudioSetNotifyEventQueue(sys_ipc_key_t key) {
    return CELL_OK;
}