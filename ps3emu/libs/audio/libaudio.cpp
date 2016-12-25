#include "libaudio.h"
#include "ps3emu/state.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"

#include <SDL2/SDL.h>

#define CELL_AUDIO_BLOCK_SAMPLES 256

#define CELL_AUDIO_PORT_2CH 2
#define CELL_AUDIO_PORT_8CH 8

#define CELL_AUDIO_BLOCK_8 8
#define CELL_AUDIO_BLOCK_16 16
#define CELL_AUDIO_BLOCK_32 32

#define CELL_AUDIO_STATUS_CLOSE 0x1010
#define CELL_AUDIO_STATUS_READY 1
#define CELL_AUDIO_STATUS_RUN 2

#define CELL_AUDIO_STATUS_CLOSE 0x1010
#define CELL_AUDIO_STATUS_READY 1
#define CELL_AUDIO_STATUS_RUN 2

namespace {
    
    struct {
        sys_event_queue_t notifyQueue = 0;
        sys_event_port_t notifyQueuePort = 0;
        uint32_t eaReadIndexAddr;
        uint32_t eaPortAddr;
        uint32_t eaRingBuffer;
        big_uint64_t* readIndexAddr;
        big_uint32_t* portAddr;
        uint8_t* ringBuffer;
        uint32_t portStatus = CELL_AUDIO_STATUS_CLOSE;
        uint32_t position = 0;
        uint32_t nBlock = 0;
        uint32_t nChannel = 0;
    } context;
    
}

int32_t cellAudioInit() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        ERROR(libs) << ssnprintf("SDL initialization failed: %s", SDL_GetError());
        exit(1);
    }
    return CELL_OK;
}

uint32_t calcBlockSize() {
    return context.nChannel * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
}

void sdlAudioCallback(void* udata, Uint8* stream, int len) {
    if (context.portStatus != CELL_AUDIO_STATUS_RUN)
        return;
    
    auto blockSize = calcBlockSize();
    auto nextBlockPosition = (*context.readIndexAddr + 1) * blockSize;
    auto toWrite = std::min<uint32_t>(nextBlockPosition - context.position, len);
    SDL_memset(stream, 0, toWrite);
    SDL_MixAudio(stream,
                 context.ringBuffer + context.position,
                 toWrite,
                 SDL_MIX_MAXVOLUME);
    context.position = (context.position + toWrite) % (blockSize * context.nBlock);
    if (context.position % blockSize == 0) {
        auto prevBlock = *context.readIndexAddr;
        *context.readIndexAddr = context.position / blockSize;
        if (prevBlock != *context.readIndexAddr) {
            sys_event_port_send(context.notifyQueuePort, 0, 0, 0);
        }
    }
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
    
    context.portStatus = CELL_AUDIO_STATUS_READY;
    context.nChannel = audioParam->nChannel;
    context.nBlock = audioParam->nBlock;
    
    context.readIndexAddr =
        g_state.heapalloc->internalAlloc<8, big_uint64_t>(&context.eaReadIndexAddr);
    context.portAddr =
        g_state.heapalloc->internalAlloc<4, big_uint32_t>(&context.eaPortAddr);
    context.ringBuffer = (uint8_t*)g_state.heapalloc->allocInternalMemory(
        &context.eaRingBuffer, calcBlockSize() * context.nBlock, 128);
    
    SDL_AudioSpec wanted;
    wanted.freq = 48000;
    wanted.format = AUDIO_F32MSB;
    wanted.channels = 2;
    wanted.samples = CELL_AUDIO_BLOCK_SAMPLES;
    wanted.callback = sdlAudioCallback;
    wanted.userdata = NULL;
    
    if (SDL_OpenAudio(&wanted, NULL) < 0) {
        ERROR(libs) << ssnprintf("Couldn't open audio: %s", SDL_GetError());
        exit(1);
    }
    
    if (portnums > 1) {
        ERROR(libs) << "too many audio ports have been opened";
        exit(1);
    }
    
    return CELL_OK;
}

int32_t cellAudioGetPortConfig(uint32_t portNum, CellAudioPortConfig* portConfig) {
    INFO(libs) << ssnprintf("cellAudioGetPortConfig(%x)", portNum);
    assert(portNum == 0);
    portConfig->readIndexAddr = context.eaReadIndexAddr;
    portConfig->status = context.portStatus;
    portConfig->nChannel = context.nChannel;
    portConfig->nBlock = context.nBlock;
    portConfig->portSize = context.nBlock * calcBlockSize();
    portConfig->portAddr = context.eaPortAddr;
    return CELL_OK;
}

int32_t cellAudioPortStart(uint32_t portNum) {
    context.portStatus = CELL_AUDIO_STATUS_RUN;
    SDL_PauseAudio(0);
    return CELL_OK;
}

int32_t cellAudioSetNotifyEventQueue(sys_ipc_key_t key) {
    INFO(libs) << ssnprintf("cellAudioSetNotifyEventQueue(%x)", key);
    context.notifyQueue = getQueueByKey(key);
    disableLogging(context.notifyQueue);
    auto res = sys_event_port_create(
        &context.notifyQueuePort, SYS_EVENT_PORT_LOCAL, SYS_EVENT_PORT_NO_NAME);
    assert(!res);
    res = sys_event_port_connect_local(context.notifyQueuePort, context.notifyQueue);
    assert(!res);
    return CELL_OK;
}

int32_t cellAudioOutSetCopyControl(CellAudioOut audioOut, CellAudioOutCopyControl control) {
    assert(audioOut == CellAudioOut::CELL_AUDIO_OUT_PRIMARY);
    return CELL_OK;
}

int32_t cellAudioCreateNotifyEventQueue(sys_event_queue_t *id, sys_ipc_key_t *key) {
    *key = 0x80004d494f323221ull;
    sys_event_queue_attr attr { 0 };
    attr.attr_protocol = SYS_SYNC_FIFO;
    strncpy(attr.name, "EmuAuNo", SYS_SYNC_NAME_SIZE);
    auto res = sys_event_queue_create(id, &attr, *key, 16);
    assert(res == CELL_OK);
    INFO(libs) << ssnprintf("cellAudioCreateNotifyEventQueue(%x, %x)", *id, *key);
    return CELL_OK;
}

int32_t cellAudioGetPortBlockTag(uint32_t portNum, uint64_t index, big_uint64_t *frameTag) {
    // TODO: implement
    *frameTag = 0;
    return CELL_OK;
}

int32_t cellAudioGetPortTimestamp(uint32_t portNum, uint64_t frameTag, big_uint64_t *timeStamp) {
    // TODO: implement
    *timeStamp = g_state.proc->getTimeBaseMicroseconds().count();
    return CELL_OK;
}
