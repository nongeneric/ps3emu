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
    _elf.map(&_mainMemory);
    _elf.link(&_mainMemory);
    _threadInitInfo = _elf.getThreadInitInfo(&_mainMemory);
    auto thread = _threads.back().get();
    initNewThread(thread, _threadInitInfo.primaryEntryPoint, _threadInitInfo.primaryStackSize);
    auto vaArgs = storeArgs(args);
    thread->setGPR(3, args.size());
    thread->setGPR(4, vaArgs);
    _threadIds.create(std::move(thread));
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
        case PPUThreadEvent::ProcessFinished:
        case PPUThreadEvent::Finished: {
            if (_threads.size() == 1 || ev.event == PPUThreadEvent::ProcessFinished) {
                _rsx.shutdown();
                return { ProcessEvent::ProcessFinished, ev.thread };
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
    {
        boost::unique_lock<boost::mutex> _(_threadMutex);
        for (auto& t : _threads) {
            t->dbgPause(true);
        }
    }
    {
        boost::unique_lock<boost::mutex> _(_cvm);
        _threadEvents.push({ event, thread });
    }
    _cv.notify_one();
}

uint64_t Process::createThread(uint32_t stackSize, uint32_t ep, uint64_t arg) {
    boost::unique_lock<boost::mutex> _(_threadMutex);
    _threads.emplace_back(std::make_unique<PPUThread>(
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }));
    auto t = _threads.back().get();
    initNewThread(t, ep, stackSize);
    t->run();
    return _threadIds.create(std::move(t));
}

void Process::initNewThread(PPUThread* thread, uint32_t entry, uint32_t stackSize) {
    auto stack = _stackBlocks.alloc(stackSize);
    auto tls = _tlsBlocks.alloc(_threadInitInfo.tlsMemSize);
    
    thread->setStackInfo(stack, stackSize);
    
    // PPU_ABI-Specifications_e
    auto mm = thread->mm();
    mm->setMemory(stack, 0, stackSize, true);
    thread->setGPR(1, stack + stackSize - sizeof(uint64_t));
    thread->setGPR(2, _threadInitInfo.tocBase);
    
    // undocumented:
    thread->setGPR(5, stack);
    thread->setGPR(6, 0);
    thread->setGPR(12, DefaultMainMemoryPageSize);
    
    mm->writeMemory(tls, _threadInitInfo.tlsBase, _threadInitInfo.tlsFileSize, true);
    mm->setMemory(tls + _threadInitInfo.tlsFileSize, 0, 
                  _threadInitInfo.tlsMemSize - _threadInitInfo.tlsFileSize, true);
    thread->setGPR(13, stack + stackSize);
    thread->setFPSCR(0);
    thread->setNIP(entry);
}

ps3_uintptr_t Process::storeArgs(std::vector<std::string> const& args) {
    auto vaArgs = _mainMemory.malloc(1 * 1024 * 1024);
    std::vector<big_uint64_t> arr;
    _mainMemory.setMemory(vaArgs, 0, (args.size() + 1) * 8, true);
    auto len = 0;
    for (auto arg : args) {
        auto vaPtr = vaArgs + (args.size() + 1) * 8 + len;
        _mainMemory.writeMemory(vaPtr, arg.data(), arg.size() + 1, true);
        arr.push_back(vaPtr);
        len += arg.size() + 1;
    }
    arr.push_back(0);
    _mainMemory.writeMemory(vaArgs, arr.data(), arr.size() * 8);
    return vaArgs;
}
