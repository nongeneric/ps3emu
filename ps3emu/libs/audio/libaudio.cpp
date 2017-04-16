#include "libaudio.h"
#include "ps3emu/state.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include "ps3emu/TimedCounter.h"
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <pulse/pulseaudio.h>
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
#define CELL_AUDIO_ERROR_AUDIOSYSTEM        CELL_ERROR_CAST(0x80310702) // error in AudioSystem
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

using namespace boost::chrono;

namespace {
    
    struct {
        sys_event_queue_t notifyQueue = 0;
        sys_event_port_t notifyQueuePort = 0;
        uint32_t eaReadIndexAddr;
        uint32_t eaPortAddr;
        big_uint64_t* readIndexAddr;
        uint8_t* portAddr;
        uint32_t portStatus = CELL_AUDIO_STATUS_CLOSE;
        uint32_t nBlock = 0;
        uint32_t nChannel = 0;
        std::vector<uint64_t> port0tags;
        std::vector<uint64_t> port0stamps;
        pa_context* pulseContext = 0;
        pa_threaded_mainloop* pulseMainLoop = 0;
        pa_stream* pulseStream = 0;
        pa_sample_spec pulseSpec;
        boost::thread playbackThread;
        boost::mutex streamStartedMutex;
        boost::condition_variable streamStartedCv;
        uint64_t samplesWritten = 0;
        std::ofstream pcmFile;
        TimedCounter counter;
    } context;
    
    struct pulseLock {
        inline pulseLock() {
            pa_threaded_mainloop_lock(context.pulseMainLoop);
        }
        inline ~pulseLock() {
            pa_threaded_mainloop_unlock(context.pulseMainLoop);
        }
    };
}

uint32_t calcBlockSize() {
    return context.nChannel * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
}

void noop(void* p) { }

void playbackLoop() {
    static std::vector<uint8_t> tempDest;
    
    for (;;) {
        {
            boost::unique_lock<boost::mutex> lock(context.streamStartedMutex);
            context.streamStartedCv.wait(lock, [&] {
                return context.portStatus == CELL_AUDIO_STATUS_RUN && context.notifyQueuePort;
            });
        }
        
        auto prevBlock = *context.readIndexAddr;
        auto curBlock = (prevBlock + 1) % context.nBlock;
        *context.readIndexAddr = curBlock;
        context.port0tags[curBlock] = context.port0tags[prevBlock] + 2;
        context.port0stamps[prevBlock] = g_state.proc->getTimeBaseMicroseconds().count();
        sys_event_port_send(context.notifyQueuePort, 0, 0, 0);
        
        auto blockSize = calcBlockSize();
        auto toWrite = CELL_AUDIO_BLOCK_SAMPLES * 2 * sizeof(float);
        auto src = (big_uint64_t*)(context.portAddr + curBlock * blockSize);
        tempDest.resize(toWrite);
        auto dest = (big_uint64_t*)&tempDest[0];
        if (context.nChannel == 8) {
            for (auto i = 0u; i < CELL_AUDIO_BLOCK_SAMPLES; ++i) {
                dest[i] = src[4*i];
            }
        } else {
            memcpy(dest, src, toWrite);
        }
        
#if TESTS
        context.pcmFile.write((char*)dest, tempDest.size());
        context.pcmFile.flush();
#endif
        
        microseconds wait(0);
        const pa_timing_info* info = nullptr;
        pa_operation* opInfo = nullptr;
        {
            pulseLock lock;
            opInfo = pa_stream_update_timing_info(context.pulseStream, NULL, NULL);
            info = pa_stream_get_timing_info(context.pulseStream);
        }
        
        if (opInfo) {
            while (pa_operation_get_state(opInfo) == PA_OPERATION_RUNNING) ;
        }
        
        {
            pulseLock lock;
            info = pa_stream_get_timing_info(context.pulseStream);
        }
        
        if (info) {
            auto sampleSize = 2 * sizeof(float);
            auto samplesPerMs = sampleSize * 48;
            auto readElapsedMs = float(info->read_index) / samplesPerMs;
            auto diffElapsedMs = float(info->write_index - info->read_index) / readElapsedMs;
            INFO(libs, perf) << ssnprintf("w: %lld r: %lld re: %g diff %g",
                                          info->write_index,
                                          info->read_index,
                                          readElapsedMs,
                                          diffElapsedMs);
                
            assert(info->write_index >= 0);
            assert(info->read_index >= 0);
            
            auto delayBytes = pa_usec_to_bytes(20000, &context.pulseSpec);
            if (info->read_index < info->write_index) {
                if ((uint32_t)info->read_index + delayBytes < (uint32_t)info->write_index) {
                    wait = microseconds((uint32_t)info->write_index - ((uint32_t)info->read_index + delayBytes));
                }
            }
            boost::this_thread::sleep_for(wait);
        
            INFO(libs, perf) << ssnprintf("wait %g ms",
                                    float(duration_cast<microseconds>(wait).count()) / 1000);
        }
        
        pulseLock lock;
        pa_stream_write(context.pulseStream, dest, tempDest.size(), noop, 0, PA_SEEK_RELATIVE);
    }
}

void waitContext() {
    for (;;) {
        pulseLock lock;
        auto state = pa_context_get_state(context.pulseContext);
        if (state == PA_CONTEXT_READY)
            return;
        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            ERROR(libs) << ssnprintf("context connection failed");
            exit(1);
        }
    }
}
int32_t cellAudioInit() {
    context.pulseMainLoop = pa_threaded_mainloop_new();
    auto api = pa_threaded_mainloop_get_api(context.pulseMainLoop);
    context.pulseContext = pa_context_new(api, "ps3emu");
    context.playbackThread = boost::thread(playbackLoop);
    auto res = pa_context_connect(context.pulseContext, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (res != PA_OK) {
        ERROR(libs) << ssnprintf("context connection failed %s", pa_strerror(res));
        exit(1);
    }
    pa_threaded_mainloop_start(context.pulseMainLoop);
    waitContext();
    return CELL_OK;
}

void overflow_handler(pa_stream *p, void *userdata) {
    ERROR(libs) << "overflow";
    exit(1);
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
    
    context.pulseSpec = { PA_SAMPLE_FLOAT32BE, 48000, 2 };    
    pa_channel_map channels;
    pa_channel_map_init_stereo(&channels);
    context.pulseStream = pa_stream_new(context.pulseContext, "emu", &context.pulseSpec, &channels);
    //2 * sizeof(float) * 512
    pa_buffer_attr attr { -1u, (uint32_t)pa_usec_to_bytes(40000, &context.pulseSpec), -1u, -1u, -1u };
    auto flags = pa_stream_flags_t(
        /*PA_STREAM_AUTO_TIMING_UPDATE | 
        PA_STREAM_INTERPOLATE_TIMING |*/
        PA_STREAM_ADJUST_LATENCY);
    auto res = pa_stream_connect_playback(context.pulseStream, NULL, &attr, flags, NULL, NULL);
    if (res != PA_OK) {
        ERROR(libs) << ssnprintf("can't connect playback stream: %s", pa_strerror(res));
        exit(1);
    }
    
    pa_stream_set_overflow_callback(context.pulseStream, overflow_handler, nullptr);
    
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
    pa_stream_trigger(context.pulseStream, NULL, NULL);
    boost::unique_lock<boost::mutex> lock(context.streamStartedMutex);
    context.portStatus = CELL_AUDIO_STATUS_RUN;
    context.streamStartedCv.notify_all();
    return CELL_OK;
}

int32_t cellAudioSetNotifyEventQueueEx(sys_ipc_key_t key, uint32_t iFlags) {
    assert(iFlags == 0);
    return cellAudioSetNotifyEventQueue(key);
}

int32_t cellAudioSetNotifyEventQueue(sys_ipc_key_t key) {
    INFO(libs) << ssnprintf("cellAudioSetNotifyEventQueue(%x)", key);
    auto queue = getQueueByKey(key);
    disableLogging(queue);
    auto res = sys_event_port_create(
        &context.notifyQueuePort, SYS_EVENT_PORT_LOCAL, 0x010003000e001c0aull);
    assert(!res); (void)res;
    res = sys_event_port_connect_local(context.notifyQueuePort, queue);
    assert(!res);
    boost::unique_lock<boost::mutex> lock(context.streamStartedMutex);
    context.notifyQueue = queue;
    context.streamStartedCv.notify_all();
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
    auto res = sys_event_queue_create(id, &attr, *key, 8);
    assert(res == CELL_OK); (void)res;
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
    ERROR(libs) << "not impl";
    // TODO: make thread safe (readIndexAddr should be atomic)
//     assert(portNum == 0);
//     auto blockSize = calcBlockSize();
//     auto block = (*context.readIndexAddr + 1) % context.nBlock;
//     auto dest = (context.portAddr + blockSize * block);
//     auto samplesSize = context.nChannel * sizeof(float) * samples;
//     auto ptr = g_state.mm->getMemoryPointer(src, samplesSize);
//     assert(volume > 0.f && volume <= 1.f);
    //SDL_MixAudioFormat(dest, ptr, AUDIO_F32MSB, samplesSize, 128.f * volume);
    return CELL_OK;
}
