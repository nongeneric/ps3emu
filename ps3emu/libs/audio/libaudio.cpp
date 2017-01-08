#include "libaudio.h"
#include "ps3emu/state.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include <SDL2/SDL.h>
#include <fstream>

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

#define CELL_ERROR_CAST(err) (static_cast<int>(err))
#define CELL_AUDIO_MAKE_ERROR(status) CELL_ERROR_MAKE_ERROR(CELL_ERROR_FACILITY_SOUND, status)
#define CELL_AUDIO_ERROR_ALREADY_INIT       CELL_ERROR_CAST(0x80310701) // aready init
#define CELL_AUDIO_ERROR_AUDIOSYSTEM        CELL_ERROR_CAST(0x80310702) // error in AudioSystem.
#define CELL_AUDIO_ERROR_NOT_INIT           CELL_ERROR_CAST(0x80310703) // not init
#define CELL_AUDIO_ERROR_PARAM              CELL_ERROR_CAST(0x80310704) // param error
#define CELL_AUDIO_ERROR_PORT_FULL          CELL_ERROR_CAST(0x80310705) // audio port is full
#define CELL_AUDIO_ERROR_PORT_ALREADY_RUN   CELL_ERROR_CAST(0x80310706) // audio port is aready run
#define CELL_AUDIO_ERROR_PORT_NOT_OPEN      CELL_ERROR_CAST(0x80310707) // audio port is close
#define CELL_AUDIO_ERROR_PORT_NOT_RUN       CELL_ERROR_CAST(0x80310708) // audio port is not run
#define CELL_AUDIO_ERROR_TRANS_EVENT        CELL_ERROR_CAST(0x80310709) // trans event error
#define CELL_AUDIO_ERROR_PORT_OPEN          CELL_ERROR_CAST(0x8031070a) // error in port open
#define CELL_AUDIO_ERROR_SHAREDMEMORY       CELL_ERROR_CAST(0x8031070b) // error in shared memory
#define CELL_AUDIO_ERROR_MUTEX              CELL_ERROR_CAST(0x8031070c) // error in mutex
#define CELL_AUDIO_ERROR_EVENT_QUEUE        CELL_ERROR_CAST(0x8031070d) // error in event queue
#define CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND  CELL_ERROR_CAST(0x8031070e) //
#define CELL_AUDIO_ERROR_TAG_NOT_FOUND          CELL_ERROR_CAST(0x8031070f) //

namespace {
    
    struct {
        sys_event_queue_t notifyQueue = 0;
        sys_event_port_t notifyQueuePort = 0;
        uint32_t eaReadIndexAddr;
        uint32_t eaPortAddr;
        big_uint64_t* readIndexAddr;
        uint8_t* portAddr;
        uint32_t portStatus = CELL_AUDIO_STATUS_CLOSE;
        uint32_t position = 0;
        uint32_t nBlock = 0;
        uint32_t nChannel = 0;
        std::vector<uint64_t> port0tags;
        std::vector<uint64_t> port0stamps;
        SDL_AudioDeviceID device;
        std::ofstream pcmFile;
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
    memcpy(stream, context.portAddr + context.position, toWrite);
#if TESTS
    context.pcmFile.write((char*)(context.portAddr + context.position), toWrite);
    context.pcmFile.flush();
#endif
    memset(context.portAddr + context.position, 0, toWrite);
    context.position = (context.position + toWrite) % (blockSize * context.nBlock);
    if (context.position % blockSize == 0) {
        auto prevBlock = *context.readIndexAddr;
        auto curBlock = context.position / blockSize;
        *context.readIndexAddr = curBlock;
        if (prevBlock != curBlock) {
            context.port0tags[curBlock] = context.port0tags[prevBlock] + 2;
            context.port0stamps[prevBlock] = g_state.proc->getTimeBaseMicroseconds().count();
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
    context.port0stamps.resize(audioParam->nBlock);
    context.port0tags.resize(audioParam->nBlock);
    std::fill(begin(context.port0tags), end(context.port0tags), 1);
    
    context.readIndexAddr =
        g_state.heapalloc->internalAlloc<8, big_uint64_t>(&context.eaReadIndexAddr);
    context.portAddr = (uint8_t*)g_state.heapalloc->allocInternalMemory(
        &context.eaPortAddr, calcBlockSize() * context.nBlock, 64u << 10);
    
    SDL_AudioSpec wanted = {0};
    wanted.freq = 48000;
    wanted.format = AUDIO_F32MSB;
    wanted.channels = 2;
    wanted.samples = CELL_AUDIO_BLOCK_SAMPLES;
    wanted.callback = sdlAudioCallback;
    wanted.userdata = NULL;
    
    context.device = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if (context.device == 0) {
        ERROR(libs) << ssnprintf("Couldn't open audio: %s", SDL_GetError());
        exit(1);
    }
    
    if (portnums > 1) {
        ERROR(libs) << "too many audio ports have been opened";
        exit(1);
    }
    
#if TESTS
    context.pcmFile = std::ofstream("/tmp/ps3emu_pcm0");
#endif
    
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
    SDL_PauseAudioDevice(context.device, 0);
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
    *key = 0x80004d494f323221ull; // "MIO22!"
    sys_event_queue_attr attr { 0 };
    attr.attr_protocol = SYS_SYNC_FIFO;
    strncpy(attr.name, "EmuAuNo", SYS_SYNC_NAME_SIZE);
    auto res = sys_event_queue_create(id, &attr, *key, 16);
    assert(res == CELL_OK);
    INFO(libs) << ssnprintf("cellAudioCreateNotifyEventQueue(%x, %x)", *id, *key);
    return CELL_OK;
}

int32_t cellAudioGetPortBlockTag(uint32_t portNum, uint64_t index, big_uint64_t *frameTag) {
    assert(portNum == 0);
    *frameTag = context.port0tags[index];
    return CELL_OK;
}

int32_t cellAudioGetPortTimestamp(uint32_t portNum, uint64_t frameTag, big_uint64_t *timeStamp) {
    assert(portNum == 0);
    for (auto i = 0u; i < context.nBlock; ++i) {
        if (context.port0tags[i] == frameTag) {
            *timeStamp = context.port0stamps[i];
            return CELL_OK;
        }
    }
    return CELL_AUDIO_ERROR_TAG_NOT_FOUND;
}

int32_t cellAudioAddData(uint32_t portNum, uint32_t src, uint32_t samples, float volume) {
    volume = g_state.th->getFPRd(0); // TODO: handle in exports.h
    // TODO: make thread safe (readIndexAddr should be atomic)
    assert(portNum == 0);
    auto blockSize = calcBlockSize();
    auto block = (*context.readIndexAddr + 1) % context.nBlock;
    auto dest = (context.portAddr + blockSize * block);
    auto samplesSize = context.nChannel * sizeof(float) * samples;
    auto ptr = g_state.mm->getMemoryPointer(src, samplesSize);
    assert(volume > 0.f && volume <= 1.f);
    SDL_MixAudioFormat(dest, ptr, AUDIO_F32MSB, samplesSize, 128.f * volume);
    return CELL_OK;
}
