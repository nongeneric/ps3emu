#include "pulse.h"
#include "ps3emu/AffinityManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include <assert.h>

using namespace boost::chrono;

#define CELL_AUDIO_PORT_2CH 2
#define CELL_AUDIO_PORT_8CH 8

#define CELL_AUDIO_BLOCK_8 8
#define CELL_AUDIO_BLOCK_16 16
#define CELL_AUDIO_BLOCK_32 32

#define CELL_AUDIO_BLOCK_SAMPLES 256

struct pulseLock {
    pa_threaded_mainloop* _loop;

    inline explicit pulseLock(pa_threaded_mainloop* loop) : _loop(loop) {}

    inline pulseLock() {
        pa_threaded_mainloop_lock(_loop);
    }

    inline ~pulseLock() {
        pa_threaded_mainloop_unlock(_loop);
    }
};

PulseBackend::PulseBackend(AudioAttributes* attributes) : _attributes(attributes) {
    _pulseSpec = { PA_SAMPLE_FLOAT32BE, 48000, 2 };
    _pulseMainLoop = pa_threaded_mainloop_new();
    auto api = pa_threaded_mainloop_get_api(_pulseMainLoop);
    _pulseContext = pa_context_new(api, "ps3emu");
    _playbackThread = boost::thread([=]{ playbackLoop(); });
    assignAffinity(_playbackThread.native_handle(), AffinityGroup::PPUHost);
    auto res = pa_context_connect(_pulseContext, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (res != PA_OK) {
        ERROR(libs) << ssnprintf("context connection failed %s", pa_strerror(res));
        exit(1);
    }
    pa_threaded_mainloop_start(_pulseMainLoop);
    waitContext();
}

void PulseBackend::openPort(unsigned id, PulsePortInfo const& info) {
    assert(info.channels == CELL_AUDIO_PORT_2CH ||
           info.channels == CELL_AUDIO_PORT_8CH);
    assert(info.blocks == CELL_AUDIO_BLOCK_8 ||
           info.blocks == CELL_AUDIO_BLOCK_16 ||
           info.blocks == CELL_AUDIO_BLOCK_32);
    pa_channel_map pchannels;
    pa_channel_map_init_stereo(&pchannels);
    auto pulseStream = pa_stream_new(_pulseContext, "emu", &_pulseSpec, &pchannels);
    //2 * sizeof(float) * 512
    pa_buffer_attr attr { -1u, (uint32_t)pa_usec_to_bytes(40000, &_pulseSpec), -1u, -1u, -1u };
    auto flags = pa_stream_flags_t(
        /*PA_STREAM_AUTO_TIMING_UPDATE |
        PA_STREAM_INTERPOLATE_TIMING |*/
        PA_STREAM_ADJUST_LATENCY);
    auto res = pa_stream_connect_playback(pulseStream, NULL, &attr, flags, NULL, NULL);
    if (res != PA_OK) {
        ERROR(libs) << ssnprintf("can't connect playback stream: %s", pa_strerror(res));
        exit(1);
    }
    auto& port = _ports[id];
    port.basic = info;
    port.pulseStream = pulseStream;
    port.mutex.reset(new boost::mutex());
}

void PulseBackend::start(unsigned id) {
    auto& port = _ports[id];
    auto lock = boost::unique_lock(*port.mutex);
    port.started = true;
    pa_stream_trigger(port.pulseStream, NULL, NULL);
}

void PulseBackend::stop(unsigned id) {
    //assert(false);
}

void PulseBackend::setNotifyQueue(uint64_t key) {
    auto lock = boost::unique_lock(_notifyQueueM);
    _notifyQueue = getQueueByKey(key);
    assert(_notifyQueue);
    auto res = sys_event_port_create(&_notifyQueuePort, SYS_EVENT_PORT_LOCAL, 0);
    assert(!res);
    (void)res;
    res = sys_event_port_connect_local(_notifyQueuePort, _notifyQueue);
    assert(!res);
    _notifyQueueCV.notify_all();
}

void PulseBackend::waitContext() {
    for (;;) {
        //pulseLock lock(_pulseMainLoop);
        auto state = pa_context_get_state(_pulseContext);
        if (state == PA_CONTEXT_READY)
            return;
        if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            ERROR(libs) << ssnprintf("context connection failed");
            exit(1);
        }
    }
}

uint32_t calcBlockSize(unsigned channels) {
    return channels * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
}

void PulseBackend::playbackLoop() {
    std::vector<uint8_t> tempDest;

    for (;;) {
        auto past = steady_clock::now();

        sys_event_port_t notifyPort = 0;
        {
            auto lock = boost::unique_lock(_notifyQueueM);
            notifyPort = _notifyQueuePort;
        }

        bool shouldSpeedUp = false;

        for (auto& [id, port] : _ports) {
            (void)id;
            auto lock = boost::unique_lock(*port.mutex);
            if (!port.started)
                continue;

            auto prevBlock = _attributes->getReadIndex(id);
            auto blockSize = calcBlockSize(port.basic.channels);
            auto toWrite = CELL_AUDIO_BLOCK_SAMPLES * 2 * sizeof(float);
            auto src = (big_uint64_t*)(port.basic.ptr + prevBlock * blockSize);
            tempDest.resize(toWrite);
            auto dest = (big_uint64_t*)&tempDest[0];
            if (port.basic.channels == 8) {
                for (auto i = 0u; i < CELL_AUDIO_BLOCK_SAMPLES; ++i) {
                    dest[i] = src[4*i];
                }
            } else {
                memcpy(dest, src, toWrite);
            }

            memset(src, 0, blockSize);

            auto res = pa_stream_write(port.pulseStream,
                                       &tempDest[0],
                                       tempDest.size(),
                                       nullptr,
                                       0,
                                       PA_SEEK_RELATIVE);
            assert(!res); (void)res;
            auto sizeLeft = pa_stream_writable_size(port.pulseStream);
            shouldSpeedUp |= sizeLeft > 2 * blockSize;

            auto curBlock = (prevBlock + 1) % port.basic.blocks;
            _attributes->setReadIndex(id, curBlock);
            _attributes->insertTimestamp(g_state.proc->getTimeBase());
        }

        if (notifyPort) {
            sys_event_port_send(notifyPort, 0, 0, 0);
        }

        auto wait = duration_cast<microseconds>(duration<unsigned, boost::ratio<256, 48000>>(1));
        auto elapsed = duration_cast<microseconds>(steady_clock::now() - past);

        if (elapsed < wait) {
            wait -= elapsed;
            if (shouldSpeedUp) {
                wait -= milliseconds(2);
            }
            boost::this_thread::sleep_for(wait);
        }
    }
}
