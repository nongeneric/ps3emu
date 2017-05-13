#pragma once

#include "../constants.h"
#include "../libs/ConcurrentBoundedQueue.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/int.h"
#include <future>
#include <vector>
#include <memory>
#include <string_view>

class Process;
class PPUThread;

struct CallbackInfo {
    bool terminate;
    std::vector<uint64_t> args;
    uint32_t toc;
    uint32_t ea;
    std::shared_ptr<std::promise<void>> promise;
};

struct CallbackThreadInitInfo;

class CallbackThread {
    CallbackInfo _lastCallback;
    ConcurrentBoundedQueue<CallbackInfo> _queue;
    uint32_t _initInfoVa;
    CallbackThreadInitInfo* _initInfo;
    bool _terminated = false;
    friend uint64_t callbackThreadQueueWait(PPUThread* ppuThread);
    
public:
    CallbackThread();
    std::future<void> schedule(std::vector<uint64_t> args, uint32_t toc, uint32_t ea);
    void terminate();
    uint64_t id();
    void ps3callInit(std::string_view name);
};

uint64_t callbackThreadQueueWait(PPUThread* ppuThread);
