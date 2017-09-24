#include "libaudio.h"
#include "ps3emu/state.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/Process.h"
#include "ps3emu/TimedCounter.h"
#include "ps3emu/HeapMemoryAlloc.h"
#include "ps3emu/AffinityManager.h"
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <pulse/pulseaudio.h>
#include <atomic>
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
#define CELL_AUDIO_PORTATTR_INITLEVEL 0x1000ull

using namespace boost::chrono;

namespace {
    
    struct PortInfo {
        std::vector<uint64_t> tags;
        std::vector<uint64_t> stamps;
        uint32_t eaReadIndexAddr;
        uint32_t eaPortAddr;
        big_uint64_t* readIndexAddr;
        uint8_t* portAddr;
        std::atomic<uint32_t> portStatus = CELL_AUDIO_STATUS_CLOSE;
        uint32_t nBlock = 0;
        uint32_t nChannel = 0;
        pa_stream* pulseStream = 0;
        std::ofstream pcmFile;
    };
    
    struct {
        sys_event_queue_t notifyQueue = 0;
        sys_event_port_t notifyQueuePort = 0;
        pa_context* pulseContext = 0;
        pa_threaded_mainloop* pulseMainLoop = 0;
        pa_sample_spec pulseSpec;
        boost::thread playbackThread;
        TimedCounter counter;
        boost::mutex streamStartedMutex;
        boost::condition_variable streamStartedCv;
        ThreadSafeIDMap<uint32_t, std::shared_ptr<PortInfo>> ports;
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

uint32_t calcBlockSize(PortInfo* info) {
    return info->nChannel * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
}

void noop(void* p) { }

void playbackLoop() {
    static std::vector<uint8_t> tempDest;

    for (;;) {
        {
            boost::unique_lock<boost::mutex> lock(context.streamStartedMutex);
            context.streamStartedCv.wait(lock, [&] {
                return context.notifyQueuePort;
            });
        }
        
        sys_event_port_send(context.notifyQueuePort, 0, 0, 0);
        
//        boost::this_thread::sleep_for(microseconds(5600));
//        continue;
        for (auto [portnum, port] : context.ports.map()) {
            (void)portnum;
            if (port->portStatus != CELL_AUDIO_STATUS_RUN)
                continue;
            
            auto prevBlock = *port->readIndexAddr;
            auto curBlock = (prevBlock + 1) % port->nBlock;
            *port->readIndexAddr = curBlock;
            port->tags[curBlock] = port->tags[prevBlock] + 2;
            port->stamps[prevBlock] = g_state.proc->getTimeBaseMicroseconds().count();
            
            auto blockSize = calcBlockSize(port.get());
            auto toWrite = CELL_AUDIO_BLOCK_SAMPLES * 2 * sizeof(float);
            auto src = (big_uint64_t*)(port->portAddr + curBlock * blockSize);
            tempDest.resize(toWrite);
            auto dest = (big_uint64_t*)&tempDest[0];
            if (port->nChannel == 8) {
                for (auto i = 0u; i < CELL_AUDIO_BLOCK_SAMPLES; ++i) {
                    dest[i] = src[4*i];
                }
            } else {
                memcpy(dest, src, toWrite);
            }
         
#if TESTS
            //port->pcmFile.write((char*)dest, tempDest.size());
            //port->pcmFile.flush();
#endif
            
            microseconds wait(0);
            const pa_timing_info* info = nullptr;
            pa_operation* opInfo = nullptr;
            {
                pulseLock lock;
                opInfo = pa_stream_update_timing_info(port->pulseStream, NULL, NULL);
                info = pa_stream_get_timing_info(port->pulseStream);
            }
            
            if (opInfo) {
                while (pa_operation_get_state(opInfo) == PA_OPERATION_RUNNING) ;
            }
            
            {
                pulseLock lock;
                info = pa_stream_get_timing_info(port->pulseStream);
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
            pa_stream_write(port->pulseStream, dest, tempDest.size(), noop, 0, PA_SEEK_RELATIVE);
        }
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
    assignAffinity(context.playbackThread.native_handle(), AffinityGroup::PPUHost);
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
    
    INFO(libs) << "cellAudioPortOpen";
    
    auto info = std::make_shared<PortInfo>();
    
    if (audioParam->attr & CELL_AUDIO_PORTATTR_INITLEVEL) {
        auto volume = union_cast<big_uint32_t, float>(audioParam->float_level);
        WARNING(libs) << ssnprintf("audio port volume %g (not implemented)", volume);
    }
    
    info->portStatus = CELL_AUDIO_STATUS_READY;
    info->nChannel = audioParam->nChannel;
    info->nBlock = audioParam->nBlock;
    info->stamps.resize(audioParam->nBlock);
    info->tags.resize(audioParam->nBlock);
    std::fill(begin(info->tags), end(info->tags), 1);
    
    auto offset = calcBlockSize(info.get()) * info->nBlock;
    auto [ptr, va] = g_state.heapalloc->alloc(offset + sizeof(big_uint64_t), 64u << 10u);
    info->portAddr = ptr;
    info->eaPortAddr = va;
    info->readIndexAddr = (big_uint64_t*)(ptr + offset);
    info->eaReadIndexAddr = va + offset;
    
    context.pulseSpec = { PA_SAMPLE_FLOAT32BE, 48000, 2 };    
    pa_channel_map channels;
    pa_channel_map_init_stereo(&channels);
    info->pulseStream = pa_stream_new(context.pulseContext, "emu", &context.pulseSpec, &channels);
    //2 * sizeof(float) * 512
    pa_buffer_attr attr { -1u, (uint32_t)pa_usec_to_bytes(40000, &context.pulseSpec), -1u, -1u, -1u };
    auto flags = pa_stream_flags_t(
        /*PA_STREAM_AUTO_TIMING_UPDATE | 
        PA_STREAM_INTERPOLATE_TIMING |*/
        PA_STREAM_ADJUST_LATENCY);
    auto res = pa_stream_connect_playback(info->pulseStream, NULL, &attr, flags, NULL, NULL);
    if (res != PA_OK) {
        ERROR(libs) << ssnprintf("can't connect playback stream: %s", pa_strerror(res));
        exit(1);
    }
    
    pa_stream_set_overflow_callback(info->pulseStream, overflow_handler, nullptr);
    
    *portNum = context.ports.create(info);
    
#if TESTS
    info->pcmFile = std::ofstream(ssnprintf("/tmp/ps3emu_pcm_%d", (uint32_t)*portNum));
#endif
    
    return CELL_OK;
}

int32_t cellAudioPortStop(uint32_t portNum) {
    WARNING(libs) << "cellAudioPortStop not implemented";
    return CELL_OK;
}

int32_t cellAudioPortClose(uint32_t portNum) {
    WARNING(libs) << "cellAudioPortClose not implemented";
    return CELL_OK;
}

int32_t cellAudioGetPortConfig(uint32_t portNum, CellAudioPortConfig* portConfig) {
    INFO(libs) << ssnprintf("cellAudioGetPortConfig(%x)", portNum);
    auto info = context.ports.get(portNum);
    portConfig->readIndexAddr = info->eaReadIndexAddr;
    portConfig->status = info->portStatus;
    portConfig->nChannel = info->nChannel;
    portConfig->nBlock = info->nBlock;
    portConfig->portSize = info->nBlock * calcBlockSize(info.get());
    portConfig->portAddr = info->eaPortAddr;
    return CELL_OK;
}

int32_t cellAudioPortStart(uint32_t portNum) {
    auto info = context.ports.get(portNum);
    pa_stream_trigger(info->pulseStream, NULL, NULL);
    info->portStatus = CELL_AUDIO_STATUS_RUN;
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
    auto info = context.ports.get(portNum);
    *frameTag = info->tags[index];
    return CELL_OK;
}

int32_t cellAudioGetPortTimestamp(uint32_t portNum, uint64_t frameTag, big_uint64_t *timeStamp) {
    auto info = context.ports.get(portNum);
    for (auto i = 0u; i < info->nBlock; ++i) {
        if (info->tags[i] == frameTag) {
            *timeStamp = info->stamps[i];
            return CELL_OK;
        }
    }
    return CELL_AUDIO_ERROR_TAG_NOT_FOUND;
}

int32_t cellAudioAddData(uint32_t portNum, uint32_t src, uint32_t samples, float volume) {
    volume = g_state.th->getFPRd(0); // TODO: handle in exports.h
    WARNING(libs) << "not impl";
    // TODO: make thread safe (readIndexAddr should be atomic)
//     assert(portNum == 1);
//     auto blockSize = calcBlockSize();
//     auto block = (*context.readIndexAddr + 1) % context.nBlock;
//     auto dest = (context.portAddr + blockSize * block);
//     auto samplesSize = context.nChannel * sizeof(float) * samples;
//     auto ptr = g_state.mm->getMemoryPointer(src, samplesSize);
//     assert(volume > 0.f && volume <= 1.f);
    //SDL_MixAudioFormat(dest, ptr, AUDIO_F32MSB, samplesSize, 128.f * volume);
    return CELL_OK;
}

int32_t cellAudioSetPortLevel(uint32_t portNum, float level) {
    WARNING(libs) << "not impl";
    return CELL_OK;
}

int32_t cellAudioAdd2chData(uint32_t portNum, uint32_t src, uint32_t samples, float volume) {
    volume = g_state.th->getFPRd(0); // TODO: handle in exports.h
    WARNING(libs) << "not impl";
    return CELL_OK;
}

int32_t cellAudioAdd6chData(uint32_t portNum, uint32_t src, float volume) {
    volume = g_state.th->getFPRd(0); // TODO: handle in exports.h
    WARNING(libs) << "not impl";
    return CELL_OK;
}
