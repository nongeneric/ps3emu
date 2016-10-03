#pragma once

#include "../constants.h"
#include "../libs/ConcurrentBoundedQueue.h"
#include <future>
#include <vector>
#include <memory>

class Process;
class PPUThread;

struct CallbackInfo {
    bool terminate;
    std::vector<uint64_t> args;
    uint32_t toc;
    uint32_t ea;
    std::shared_ptr<std::promise<void>> promise;
};

class CallbackThread {
    CallbackInfo _lastCallback;
    ConcurrentBoundedQueue<CallbackInfo> _queue;
    uint64_t _id = 0;
    friend uint64_t callbackThreadQueueWait(PPUThread* ppuThread);
    
public:
    CallbackThread(Process* proc);
    std::future<void> schedule(std::vector<uint64_t> args, uint32_t toc, uint32_t ea);
    void terminate();
    uint64_t id();
};

uint64_t callbackThreadQueueWait(PPUThread* ppuThread);
