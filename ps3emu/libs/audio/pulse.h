#pragma once

#include "ps3emu/int.h"
#include "ps3emu/libs/sync/queue.h"
#include "AudioAttributes.h"
#include <pulse/pulseaudio.h>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <map>
#include <memory>

struct PulsePortInfo {
    uint8_t* ptr = nullptr;
    unsigned blocks = 0;
    unsigned channels = 0;
    float level = 0;
};

struct PulsePortInfoEx {
    PulsePortInfo basic;
    pa_stream* pulseStream = 0;
    bool started = false;
    std::unique_ptr<boost::mutex> mutex;
};

class PulseBackend {
    sys_event_queue_t _notifyQueue = 0;
    sys_event_port_t _notifyQueuePort = 0;
    pa_context* _pulseContext = 0;
    pa_threaded_mainloop* _pulseMainLoop = 0;
    pa_sample_spec _pulseSpec;
    boost::thread _playbackThread;
    std::map<unsigned, PulsePortInfoEx> _ports;
    boost::mutex _notifyQueueM;
    boost::condition_variable _notifyQueueCV;
    AudioAttributes* _attributes;
    std::atomic<bool> _stop = false;

    void waitContext();
    void playbackLoop();
    void dumpStats();

public:
    PulseBackend(AudioAttributes* attributes);
    void openPort(unsigned id, PulsePortInfo const& info);
    void start(unsigned id);
    void stop(unsigned id);
    void setNotifyQueue(uint64_t key);
    void quit();
};
