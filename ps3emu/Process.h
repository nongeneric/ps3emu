#pragma once

#include "rsx/Rsx.h"
#include "ELFLoader.h"
#include "MainMemory.h"
#include "PPUThread.h"

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
    boost::condition_variable _cv;
    std::queue<PPUThreadEventInfo> _threadEvents;
    
    Rsx _rsx;
    ELFLoader _elf;
    MainMemory _mainMemory;
    std::vector<std::unique_ptr<PPUThread>> _threads;
    bool _firstRun = true;
    void ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event);
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
};