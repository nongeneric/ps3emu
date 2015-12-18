#include "Process.h"

#include <boost/thread/locks.hpp>

MainMemory* Process::mm() {
    return &_mainMemory;
}

ELFLoader* Process::elfLoader() {
    return &_elf;
}

Rsx* Process::rsx() {
    return &_rsx;
}

void Process::init(std::string elfPath, std::vector<std::string> args) {
    _threads.emplace_back(std::make_unique<PPUThread>(
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }));
    _mainMemory.setRsx(&_rsx);
    _elf.load(elfPath);
    _elf.map(_threads.back().get(), args);
    _elf.link(&_mainMemory);
}

void Process::terminate() {
    
}

ProcessEventInfo Process::run() {
    if (_firstRun) {
        _firstRun = false;
        _threads.back()->run();
    } else {
        for (auto& t : _threads) {
            t->dbgPause(false);
        }
    }
    boost::unique_lock<boost::mutex> lock(_cvm);
    _cv.wait(lock, [=] { return !_threadEvents.empty(); });
    auto ev = _threadEvents.front();
    _threadEvents.pop();
    switch (ev.event) {
        case PPUThreadEvent::Started: return { ProcessEvent::ThreadCreated, ev.thread };
        case PPUThreadEvent::Finished: {
            if (_threads.size() == 1) {
                _rsx.shutdown();
                return { ProcessEvent::ProcessFinished, nullptr };
            }
            return { ProcessEvent::ThreadFinished, ev.thread };
        }
        case PPUThreadEvent::Breakpoint: return { ProcessEvent::Breakpoint, ev.thread };
        case PPUThreadEvent::InvalidInstruction: return { ProcessEvent::InvalidInstruction, ev.thread };
        case PPUThreadEvent::MemoryAccessError: return { ProcessEvent::MemoryAccessError, ev.thread };
        case PPUThreadEvent::Failure: return { ProcessEvent::Failure, ev.thread };
        default: throw std::runtime_error("unknown event");
    }
}

void Process::ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event) {
    for (auto& t : _threads) {
        t->dbgPause(true);
    }
    {
        boost::unique_lock<boost::mutex> _(_cvm);
        _threadEvents.push({ event, thread });
    }
    _cv.notify_one();
}
