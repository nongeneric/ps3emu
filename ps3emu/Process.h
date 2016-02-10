#pragma once

#include "rsx/Rsx.h"
#include "ppu/PPUThread.h"
#include "spu/SPUThread.h"
#include "MemoryBlockManager.h"
#include "IDMap.h"
#include "../libs/ConcurrentQueue.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/variant.hpp>

#include <memory>
#include <string>
#include <vector>

class SPUThread;

struct ProcessFinishedEvent {
    
};

struct PPUThreadStartedEvent {
    PPUThread* thread;
};

struct PPUThreadFinishedEvent {
    PPUThread* thread;
};

struct PPUBreakpointEvent {
    PPUThread* thread;
};

struct PPUSingleStepBreakpointEvent {
    PPUThread* thread;
};

struct PPUInvalidInstructionEvent {
    PPUThread* thread;
};
    
struct SPUThreadStartedEvent {
    SPUThread* thread;
};

struct SPUThreadFinishedEvent {
    SPUThread* thread;
};

struct SPUInvalidInstructionEvent {
    SPUThread* thread;
};

struct SPUBreakpointEvent {
    SPUThread* thread;
};

struct SPUSingleStepBreakpointEvent {
    SPUThread* thread;
};

struct MemoryAccessErrorEvent {
    PPUThread* thread;
};

struct PPUThreadFailureEvent {
    PPUThread* thread;
};

struct SPUThreadFailureEvent {
    SPUThread* thread;
};

using Event = boost::variant<ProcessFinishedEvent,
                             PPUThreadStartedEvent,
                             PPUThreadFinishedEvent,
                             PPUBreakpointEvent,
                             PPUSingleStepBreakpointEvent,
                             PPUInvalidInstructionEvent,
                             SPUThreadStartedEvent,
                             SPUThreadFinishedEvent,
                             SPUInvalidInstructionEvent,
                             SPUBreakpointEvent,
                             SPUSingleStepBreakpointEvent,
                             MemoryAccessErrorEvent,
                             PPUThreadFailureEvent,
                             SPUThreadFailureEvent>;

struct PPUThreadEventInfo {
    PPUThreadEvent event;
    PPUThread* thread;
};

struct SPUThreadEventInfo {
    SPUThreadEvent event;
    SPUThread* thread;
};

using ThreadEvent = boost::variant<PPUThreadEventInfo, SPUThreadEventInfo>;

struct ThreadInitInfo;
class Rsx;
class ELFLoader;
class MainMemory;
class ContentManager;
class InternalMemoryManager;
class CallbackThread;

class Process {
    ConcurrentFifoQueue<ThreadEvent> _eventQueue;
    std::unique_ptr<ThreadInitInfo> _threadInitInfo;
    MemoryBlockManager<StackArea, StackAreaSize, 1 * 1024 * 1024> _stackBlocks;
    MemoryBlockManager<TLSArea, TLSAreaSize, 1 * 1024 * 1024> _tlsBlocks;
    std::unique_ptr<Rsx> _rsx;
    std::unique_ptr<ELFLoader> _elf;
    std::unique_ptr<MainMemory> _mainMemory;
    std::unique_ptr<ContentManager> _contentManager;
    std::unique_ptr<InternalMemoryManager> _internalMemoryManager;
    std::unique_ptr<CallbackThread> _callbackThread;
    std::vector<std::unique_ptr<PPUThread>> _threads;
    boost::mutex _ppuThreadMutex;
    std::vector<std::unique_ptr<SPUThread>> _spuThreads;
    boost::mutex _spuThreadMutex;
    IDMap<uint64_t, PPUThread*> _threadIds;
    IDMap<uint32_t, SPUThread*> _spuThreadIds;
    bool _firstRun = true;
    void ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event);
    void initNewThread(PPUThread* thread, ps3_uintptr_t entryDescriptorVa, uint32_t stackSize);
    ps3_uintptr_t storeArgs(std::vector<std::string> const& args);
    void dbgPause(bool pause);
    Process(Process&) = delete;
    Process& operator=(Process&) = delete;
    
public:
    Process();
    ~Process();
    Rsx* rsx();
    ELFLoader* elfLoader();
    MainMemory* mm();
    ContentManager* contentManager();
    InternalMemoryManager* internalMemoryManager();
    void init(std::string elfPath, std::vector<std::string> args);
    Event run();
    uint64_t createThread(uint32_t stackSize,
                          ps3_uintptr_t entryPointDescriptorVa,
                          uint64_t arg);
    uint64_t createInterruptThread(uint32_t stackSize,
                                   ps3_uintptr_t entryPointDescriptorVa,
                                   uint64_t arg);
    PPUThread* getThread(uint64_t id);
    uint32_t createSpuThread(std::string name);
    SPUThread* getSpuThread(uint32_t id);
    CallbackThread* getCallbackThread();
    void destroySpuThread(SPUThread* thread);
    std::vector<PPUThread*> dbgPPUThreads();
    std::vector<SPUThread*> dbgSPUThreads();
};