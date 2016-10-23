#pragma once

#include "rsx/Rsx.h"
#include "ppu/PPUThread.h"
#include "spu/SPUThread.h"
#include "ELFLoader.h"
#include "IDMap.h"
#include "libs/ConcurrentQueue.h"
#include "OneTimeEvent.h"

#include <boost/chrono.hpp>
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

struct PPUModuleLoadedEvent {
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
                             SPUThreadFailureEvent,
                             PPUModuleLoadedEvent>;

struct PPUThreadEventInfo {
    PPUThreadEvent event;
    PPUThread* thread;
};

struct SPUThreadEventInfo {
    SPUThreadEvent event;
    SPUThread* thread;
};

struct ThreadEvent {
    boost::variant<PPUThreadEventInfo, SPUThreadEventInfo> info;
    std::shared_ptr<OneTimeEvent> promise;
};

struct ThreadInitInfo;
class Rsx;
class MainMemory;
class ContentManager;
class InternalMemoryManager;
class CallbackThread;

struct ModuleSegment {
    ModuleSegment(std::shared_ptr<ELFLoader> elf, unsigned index, ps3_uintptr_t va, uint32_t size)
        : elf(elf), index(index), va(va), size(size) {}
    std::shared_ptr<ELFLoader> elf;
    unsigned index;
    ps3_uintptr_t va;
    uint32_t size;
};

class Process {
    ConcurrentFifoQueue<ThreadEvent> _eventQueue;
    std::unique_ptr<ThreadInitInfo> _threadInitInfo;
    std::unique_ptr<Rsx> _rsx;
    std::shared_ptr<ELFLoader> _elf;
    std::unique_ptr<MainMemory> _mainMemory;
    std::unique_ptr<ContentManager> _contentManager;
    std::unique_ptr<InternalMemoryManager> _internalMemoryManager;
    std::unique_ptr<InternalMemoryManager> _heapMemoryManager;
    std::unique_ptr<InternalMemoryManager> _stackBlocks;
    std::unique_ptr<CallbackThread> _callbackThread;
    std::vector<std::unique_ptr<PPUThread>> _threads;
    boost::recursive_mutex _ppuThreadMutex;
    std::vector<std::shared_ptr<SPUThread>> _spuThreads;
    boost::recursive_mutex _spuThreadMutex;
    IDMap<uint64_t, PPUThread*, 0> _threadIds;
    IDMap<uint32_t, std::shared_ptr<SPUThread>> _spuThreadIds;
    bool _firstRun = true;
    boost::chrono::high_resolution_clock::time_point _systemStart;
    std::vector<ModuleSegment> _segments;
    std::vector<std::shared_ptr<ELFLoader>> _prxs;
    std::vector<StolenFuncInfo> _stolenInfos;
    bool _processFinished = false;
    bool _callbackThreadFinished = false;
    void ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event);
    void initPrimaryThread(PPUThread* thread, ps3_uintptr_t entryDescriptorVa, uint32_t stackSize);
    void initNewThread(PPUThread* thread,
                       ps3_uintptr_t entryDescriptorVa,
                       uint32_t stackSize,
                       uint32_t tls);
    ps3_uintptr_t storeArgs(std::vector<std::string> const& args);
    void loadPrxStore();
    Process(Process&) = delete;
    Process& operator=(Process&) = delete;
    
public:
    Process();
    ~Process();
    Rsx* rsx();
    ELFLoader* elfLoader();
    void init(std::string elfPath, std::vector<std::string> args);
    uint32_t loadPrx(std::string path);
    Event run();
    uint64_t createThread(uint32_t stackSize,
                          ps3_uintptr_t entryPointDescriptorVa,
                          uint64_t arg,
                          std::string name,
                          uint32_t tls,
                          bool start = true);
    uint64_t createInterruptThread(uint32_t stackSize,
                                   ps3_uintptr_t entryPointDescriptorVa,
                                   uint64_t arg,
                                   std::string name,
                                   uint32_t tls,
                                   bool start = true);
    PPUThread* getThread(uint64_t id);
    uint32_t createSpuThread(std::string name);
    std::shared_ptr<SPUThread> getSpuThread(uint32_t id);
    std::shared_ptr<SPUThread> getSpuThreadBySpuNum(uint32_t spuNum);
    CallbackThread* getCallbackThread();
    std::vector<PPUThread*> dbgPPUThreads();
    std::vector<SPUThread*> dbgSPUThreads();
    uint64_t getFrequency();
    uint64_t getTimeBase();
    boost::chrono::microseconds getTimeBaseMicroseconds();
    boost::chrono::nanoseconds getTimeBaseNanoseconds();
    std::vector<ModuleSegment>& getSegments();
    std::vector<std::shared_ptr<ELFLoader>> loadedModules();
    StolenFuncInfo getStolenInfo(uintptr_t ncallIndex);
    void dbgPause(bool pause);
};

int32_t executeExportedFunction(uint32_t imageBase,
                                size_t args,
                                ps3_uintptr_t argp,
                                ps3_uintptr_t modres, // big_int32_t*
                                PPUThread* thread,
                                const char* name);
int32_t EmuInitLoadedPrxModules(PPUThread* thread);
uint32_t findExportedModuleFunction(uint32_t imageBase, const char* name);
