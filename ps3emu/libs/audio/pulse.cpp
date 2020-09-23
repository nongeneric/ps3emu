#include "pulse.h"
#include "ps3emu/AffinityManager.h"
#include "ps3emu/log.h"
#include "ps3emu/Process.h"
#include "ps3emu/Config.h"
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <boost/range/irange.hpp>

using namespace std::chrono;

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
        ERROR(audio) << sformat("context connection failed");
        exit(1);
    }
}

PulseBackend::PulseBackend(AudioAttributes* attributes) : _attributes(attributes) {
    _pulseSpec = { PA_SAMPLE_FLOAT32BE, 48000, 2 };
    _pulseMainLoop = pa_threaded_mainloop_new();
    auto api = pa_threaded_mainloop_get_api(_pulseMainLoop);
    _pulseContext = pa_context_new(api, "ps3emu");
    _playbackThread = boost::thread([this]{ playbackLoop(); });
    assignAffinity(_playbackThread.native_handle(), AffinityGroup::PPUHost);
    auto res = pa_context_connect(_pulseContext, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (res != PA_OK) {
        ERROR(audio) << sformat("context connection failed {}", pa_strerror(res));
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
        ERROR(audio) << sformat("can't connect playback stream: {}", pa_strerror(res));
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

struct EightChannels {
    big_uint32_t left;
    big_uint32_t right;
    big_uint32_t center;
    big_uint32_t lfe;
    big_uint32_t leftSurround;
    big_uint32_t rightSurround;
    big_uint32_t leftExtend;
    big_uint32_t rightExtend;
};

struct TwoChannels {
    big_uint32_t left;
    big_uint32_t right;
};

void PulseBackend::playbackLoop() {
    log_set_thread_name("emu_audioloop");

    std::vector<uint8_t> tempDest;

    struct FCapture {
        FILE* pulse{};
        FILE* raw{};
    };
    std::array<FCapture, 8> fCapture;

    if (g_state.config->captureAudio) {
        for (auto i = 0u; i < fCapture.size(); ++i) {
            fCapture[i].pulse = fopen(sformat("/tmp/ps3emu_audio_port{}.bin", i).c_str(), "w");
            fCapture[i].raw = fopen(sformat("/tmp/ps3emu_audio_raw{}.bin", i).c_str(), "w");
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
            DETAIL(audio) << sformat("audioLoop notifying");
            sys_event_port_send(notifyPort, 0, 0, 0);
        }

        nap(std::chrono::milliseconds(4));
        dumpStats();
        bool shouldSpeedUp = false;

        for (auto& [id, port] : _ports) {
            auto lock = boost::unique_lock(*port.mutex);
            if (!port.started)
                continue;

            auto curBlock = _attributes->getReadIndex(id);
            auto blockSize = calcBlockSize(port.basic.channels);
            auto toWrite = CELL_AUDIO_BLOCK_SAMPLES * 2 * sizeof(float);
            auto src = (EightChannels*)(port.basic.ptr + curBlock * blockSize);
            tempDest.resize(toWrite);
            auto dest = (TwoChannels*)&tempDest[0];
            if (port.basic.channels == 8) {
                for (auto i = 0u; i < CELL_AUDIO_BLOCK_SAMPLES; ++i) {
                    auto& ch8 = src[i];
                    dest[i].left = bit_cast<uint32_t>(
                        bit_cast<float>((uint32_t)ch8.left) +
                        bit_cast<float>((uint32_t)ch8.center) +
                        bit_cast<float>((uint32_t)ch8.leftSurround) +
                        bit_cast<float>((uint32_t)ch8.leftExtend));
                    dest[i].right = bit_cast<uint32_t>(
                        bit_cast<float>((uint32_t)ch8.right) +
                        bit_cast<float>((uint32_t)ch8.center) +
                        bit_cast<float>((uint32_t)ch8.rightSurround) +
                        bit_cast<float>((uint32_t)ch8.rightExtend));
                }
            } else {
                memcpy(dest, src, toWrite);
            }

            memset(src, 0, blockSize);

            if (g_state.config->captureAudio) {
                auto f = fCapture[id - AudioAttributes::portHwBase];
                fwrite(&tempDest[0], 1, tempDest.size(), f.pulse);
                fwrite(src, 1, CELL_AUDIO_BLOCK_SAMPLES * port.basic.channels * sizeof(float), f.raw);
                fflush(f.pulse);
                fflush(f.raw);
            }

            pulseLock plock(_pulseMainLoop);

            pa_stream_write(port.pulseStream,
                            &tempDest[0],
                            tempDest.size(),
                            nullptr,
                            0,
                            PA_SEEK_RELATIVE);


            auto sizeLeft = pa_stream_writable_size(port.pulseStream);
            shouldSpeedUp |= sizeLeft > 2 * blockSize;
        }

        auto wait = duration_cast<microseconds>(duration<unsigned, std::ratio<256, 48000>>(1));
        auto elapsed = duration_cast<microseconds>(steady_clock::now() - past);

        if (elapsed < wait) {
            wait -= elapsed;
            auto speedup = milliseconds(4);
            if (shouldSpeedUp) {
                if (wait > speedup) {
                    wait -= speedup;
                } else {
                    wait = {};
                }
            }
            nap(wait);
        }
    }
}

void PulseBackend::dumpStats() {
    if (!log_should(log_detail, log_type_t::audio))
        return;

    for (auto& [id, port] : _ports) {
        auto blockSize = calcBlockSize(port.basic.channels);
        auto readIndex = _attributes->getReadIndex(id);
        std::string stats = sformat("Port {} stats: ", id);
        for (auto block : boost::irange(port.basic.blocks)) {
            auto src = port.basic.ptr + block * blockSize;
            auto rsrc = std::reverse_iterator(src + blockSize);
            auto rsrcEnd = std::reverse_iterator(src);
            auto rit = std::find_if(rsrc, rsrcEnd, [](auto ch) { return ch != 0; });
            auto zeroes = std::distance(rsrc, rit);
            auto capacity = (int)((1 - (float)zeroes / blockSize) * 100);
            stats += sformat("{}{:3} ", block == readIndex ? "R" : " ", capacity);
        }
        DETAIL(audio) << stats;
    }
}
