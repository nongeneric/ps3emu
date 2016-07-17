#pragma once

#include "PPUThread.h"
#include "../libs/ConcurrentQueue.h"

class SPUThread;

enum class InterruptType {
    Disestablish,
    Spu
};

struct InterruptInfo {
    InterruptType type;
};

class InterruptPPUThread : public PPUThread {
    ConcurrentFifoQueue<InterruptInfo> _queue;
    uint32_t _entry;
    uint32_t _mask2;
    uint64_t _arg;
    void innerLoop() override;
    void handler(PPUThread* thread, PPUThreadEvent event);
    SPUThread* _establishedThread = nullptr;
    
public:
    InterruptPPUThread(Process* proc,
                       std::function<void(PPUThread*, PPUThreadEvent)> eventHandler);
    void establish(SPUThread* thread);
    void disestablish();
    void setEntry(uint32_t entry);
    void setMask2(uint32_t mask);
    void endOfInterrupt();
    void setArg(uint64_t arg) override;
};
