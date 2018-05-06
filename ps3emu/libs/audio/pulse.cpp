#include "pulse.h"
#include "ps3emu/AffinityManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include "ps3emu/Config.h"
#include <pthread.h>
#include <sched.h>
#include <assert.h>

using namespace boost::chrono;

#define CELL_AUDIO_PORT_2CH 2
#define CELL_AUDIO_PORT_8CH 8

#define CELL_AUDIO_BLOCK_8 8
#define CELL_AUDIO_BLOCK_16 16
#define CELL_AUDIO_BLOCK_32 32

#define CELL_AUDIO_BLOCK_SAMPLES 256

struct pulseLock {
    pa_threaded_mainloop* _loop = nullptr;

    inline explicit pulseLock(pa_threaded_mainloop* loop) : _loop(loop) {
        pa_threaded_mainloop_lock(_loop);
    }

    inline ~pulseLock() {
        pa_threaded_mainloop_unlock(_loop);
    }
};

void pa_state_cb(pa_context* c, void* userdata) {
    pa_context_state_t state;
    auto pa_ready = static_cast<int*>(userdata);
    state = pa_context_get_state(c);
    switch (state) {
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED: *pa_ready = 2; break;
        case PA_CONTEXT_READY: *pa_ready = 1; break;
        default: break;
    }
}

void PulseBackend::waitContext() {
    int status = 0;
    pa_context_set_state_callback(_pulseContext, pa_state_cb, &status);
    pa_threaded_mainloop_start(_pulseMainLoop);
    while (!status) ums_sleep(1000);
    if (status == 2) {
        ERROR(libs) << ssnprintf("context connection failed");
        exit(1);
    }
}

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

void PulseBackend::quit() {
    _stop = true;
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

uint32_t calcBlockSize(unsigned channels) {
    return channels * sizeof(float) * CELL_AUDIO_BLOCK_SAMPLES;
}

void PulseBackend::playbackLoop() {
    std::vector<uint8_t> tempDest;

    std::array<FILE*, 8> fCapture;
    if (g_state.config->captureAudio) {
        for (auto i = 0u; i < fCapture.size(); ++i) {
            fCapture[i] = fopen(ssnprintf("/tmp/ps3emu_audio_port%d.bin", i).c_str(), "w");
        }
    }

    while (!_stop) {
        auto past = steady_clock::now();

        sys_event_port_t notifyPort = 0;
        {
            auto lock = boost::unique_lock(_notifyQueueM);
            notifyPort = _notifyQueuePort;
        }

        for (auto& [id, port] : _ports) {
            (void)id;
            auto lock = boost::unique_lock(*port.mutex);
            if (!port.started)
                continue;
            auto prevBlock = _attributes->getReadIndex(id);
            auto curBlock = (prevBlock + 1) % port.basic.blocks;
            _attributes->setReadIndex(id, curBlock);

            // a little delay, to simulate ps3
            auto delta = 20000 * g_state.proc->getFrequency() / 1000000;
            _attributes->insertTimestamp(g_state.proc->getTimeBase() - delta);
        }

        if (notifyPort) {
            INFO(libs) << ssnprintf("audioLoop notifying");
            sys_event_port_send(notifyPort, 0, 0, 0);
        }

        bool shouldSpeedUp = false;

        for (auto& [id, port] : _ports) {
            (void)id;
            auto lock = boost::unique_lock(*port.mutex);
            if (!port.started)
                continue;

            auto curBlock = _attributes->getReadIndex(id);
            auto blockSize = calcBlockSize(port.basic.channels);
            auto toWrite = CELL_AUDIO_BLOCK_SAMPLES * 2 * sizeof(float);
            auto src = (big_uint64_t*)(port.basic.ptr + curBlock * blockSize);
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

            pulseLock plock(_pulseMainLoop);

            auto res = pa_stream_write(port.pulseStream,
                                       &tempDest[0],
                                       tempDest.size(),
                                       nullptr,
                                       0,
                                       PA_SEEK_RELATIVE);

            if (g_state.config->captureAudio) {
                auto f = fCapture[id - AudioAttributes::portHwBase];
                fwrite(&tempDest[0], 1, tempDest.size(), f);
            }

            (void)res;
            if (res) {
                ERROR(libs) << ssnprintf("libaudio fatal: %s", pa_strerror(res));
                exit(1);
            }
            auto sizeLeft = pa_stream_writable_size(port.pulseStream);
            shouldSpeedUp |= sizeLeft > 2 * blockSize;
        }

        auto wait = duration_cast<microseconds>(duration<unsigned, boost::ratio<256, 48000>>(1));
        auto elapsed = duration_cast<microseconds>(steady_clock::now() - past);

        if (elapsed < wait) {
            wait -= elapsed;
            if (shouldSpeedUp) {
                wait -= milliseconds(1);
            }
            boost::this_thread::sleep_for(wait);
        }
    }
}
