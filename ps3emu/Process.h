#pragma once

#include "rsx/Rsx.h"
#include "ELFLoader.h"
#include "MainMemory.h"
#include "PPUThread.h"
#include "MemoryBlockManager.h"
#include "IDMap.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <memory>
#include <string>
#include <vector>
#include <queue>

enum class ProcessEvent {
    Breakpoint,
    ProcessFinished,
    ThreadCreated,
    ThreadFinished,
    InvalidInstruction,
    MemoryAccessError,
    Failure
};

struct ProcessEventInfo {
    ProcessEvent event;
    PPUThread* thread;
};

struct PPUThreadEventInfo {
    PPUThreadEvent event;
    PPUThread* thread;
};

class Process {
    boost::mutex _cvm;
    boost::mutex _threadMutex;
    boost::condition_variable _cv;
    ThreadInitInfo _threadInitInfo;
    MemoryBlockManager<StackArea, StackAreaSize, 4 * 1024> _stackBlocks;
    MemoryBlockManager<TLSArea, TLSAreaSize, 64 * 1024> _tlsBlocks;
    std::queue<PPUThreadEventInfo> _threadEvents;
    
    Rsx _rsx;
    ELFLoader _elf;
    MainMemory _mainMemory;
    std::vector<std::unique_ptr<PPUThread>> _threads;
    IDMap<uint64_t, PPUThread*> _threadIds;
    bool _firstRun = true;
    void ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event);
    void initNewThread(PPUThread* thread, uint32_t entry, uint32_t stackSize);
    ps3_uintptr_t storeArgs(std::vector<std::string> const& args);
    Process(Process&) = delete;
    Process& operator=(Process&) = delete;
public:
    Process() = default;
    Rsx* rsx();
    ELFLoader* elfLoader();
    MainMemory* mm();
    void init(std::string elfPath, std::vector<std::string> args);
    void terminate();
    ProcessEventInfo run();
    uint64_t createThread(uint32_t stackSize, uint32_t ep, uint64_t arg);
};