#include "Process.h"

#include <boost/thread/locks.hpp>
#include <boost/log/trivial.hpp>

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
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }, true));
    _mainMemory.setRsx(&_rsx);
    _elf.load(elfPath);
    _elf.map(&_mainMemory);
    _elf.link(&_mainMemory);
    _contentManager.setElfPath(elfPath);
    _threadInitInfo = _elf.getThreadInitInfo(&_mainMemory);
    auto thread = _threads.back().get();
    initNewThread(thread,
                  _threadInitInfo.entryPointDescriptorVa,
                  _threadInitInfo.primaryStackSize);
    auto vaArgs = storeArgs(args);
    thread->setGPR(3, args.size());
    thread->setGPR(4, vaArgs);
    _threadIds.create(std::move(thread));
}

void Process::dbgPause(bool pause) {
#ifdef DEBUG
    boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
    for (auto& t : _threads) {
        t->dbgPause(pause);
    }
    for (auto& t : _spuThreads) {
        t->dbgPause(pause);
    }
#endif
}

Event Process::run() {
    if (_firstRun) {
        _firstRun = false;
        _threads.back()->run();
    }

    ThreadEvent threadEvent;
    size_t num;
    _eventQueue.tryReceive(&threadEvent, 1, &num);
    if (!num) {
        dbgPause(false);
        threadEvent = _eventQueue.receive(0);
    }
    dbgPause(true);

    if (auto ev = boost::get<PPUThreadEventInfo>(&threadEvent)) {
        switch (ev->event) {
            case PPUThreadEvent::Started: return PPUThreadStartedEvent{ev->thread};
            case PPUThreadEvent::ProcessFinished:
            case PPUThreadEvent::Finished: {
                if (ev->event == PPUThreadEvent::ProcessFinished) {
                    for (auto& t : _threads) {
                        t->join();
                    }
                    _rsx.shutdown();
                    return ProcessFinishedEvent();
                }
                return PPUThreadFinishedEvent{ev->thread};
            }
            case PPUThreadEvent::Breakpoint: return PPUBreakpointEvent{ev->thread};
            case PPUThreadEvent::InvalidInstruction:
                return PPUInvalidInstructionEvent{ev->thread};
            case PPUThreadEvent::MemoryAccessError:
                return MemoryAccessErrorEvent{ev->thread};
            case PPUThreadEvent::Joined: {
                boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
                auto it =
                    std::find_if(begin(_threads),
                                 end(_threads),
                                 [=](auto& th) { return th.get() == ev->thread; });
                assert(it != end(_threads));
                _threads.erase(it);
            }
            case PPUThreadEvent::Failure: return PPUThreadFailureEvent{ev->thread};
        }
    } else if (auto ev = boost::get<SPUThreadEventInfo>(&threadEvent)) {
        switch (ev->event) {
            case SPUThreadEvent::Breakpoint: return SPUBreakpointEvent{ev->thread};
            case SPUThreadEvent::Started: return SPUThreadStartedEvent{ev->thread};
            case SPUThreadEvent::Finished: return SPUThreadFinishedEvent{ev->thread};
            case SPUThreadEvent::InvalidInstruction:
                return SPUInvalidInstructionEvent{ev->thread};
            case SPUThreadEvent::Failure: return SPUThreadFailureEvent{ev->thread};
        }
    }
    throw std::runtime_error("unknown event");
}

uint64_t Process::createThread(uint32_t stackSize,
                               ps3_uintptr_t entryPointDescriptorVa,
                               uint64_t arg) {
    boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
    _threads.emplace_back(std::make_unique<PPUThread>(
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }, false));
    auto t = _threads.back().get();
    initNewThread(t, entryPointDescriptorVa, stackSize);
    t->setGPR(3, arg);
    auto id = _threadIds.create(std::move(t));
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("thread %d created", id);
    t->run();
    return id;
}

void Process::initNewThread(PPUThread* thread, ps3_uintptr_t entryDescriptorVa, uint32_t stackSize) {
    auto stack = _stackBlocks.alloc(stackSize);
    auto tls = _tlsBlocks.alloc(_threadInitInfo.tlsMemSize);
    
    thread->setStackInfo(stack, stackSize);
    
    // PPU_ABI-Specifications_e
    auto mm = thread->mm();
    
    fdescr entryDescr;
    mm->readMemory(entryDescriptorVa, &entryDescr, sizeof(fdescr));
    
    mm->setMemory(stack, 0, stackSize, true);
    thread->setGPR(1, stack + stackSize - 2 * sizeof(uint64_t));
    thread->setGPR(2, entryDescr.tocBase);
    
    // undocumented:
    thread->setGPR(5, stack);
    thread->setGPR(6, 0);
    thread->setGPR(8, entryDescriptorVa);
    thread->setGPR(12, DefaultMainMemoryPageSize);
    
    mm->writeMemory(tls, _threadInitInfo.tlsBase, _threadInitInfo.tlsFileSize, true);
    mm->setMemory(tls + _threadInitInfo.tlsFileSize, 0, 
                  _threadInitInfo.tlsMemSize - _threadInitInfo.tlsFileSize, true);
    thread->setGPR(13, tls + 0x7000);
    thread->setFPSCR(0);
    thread->setNIP(entryDescr.va);
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

PPUThread* Process::getThread(uint64_t id) {
    boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
    return _threadIds.get(id);
}

ContentManager* Process::contentManager() {
    return &_contentManager;
}

uint32_t Process::createSpuThread(std::string name) {
    boost::unique_lock<boost::mutex> _(_spuThreadMutex);
    _spuThreads.emplace_back(
        std::make_unique<SPUThread>(this,
                                    name,
                                    [=](auto t, auto e) {
                                        _eventQueue.send(SPUThreadEventInfo{e, t});
                                    }));
    auto t = _spuThreads.back().get();
    auto id = _spuThreadIds.create(std::move(t));
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("spu thread %d created", id);
    return id;
}

SPUThread* Process::getSpuThread(uint32_t id) {
    boost::unique_lock<boost::mutex> _(_spuThreadMutex);
    return _spuThreadIds.get(id);
}

Process::Process() : _eventQueue(QueueReceivingOrder::Fifo) {}

void Process::ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event) {
    _eventQueue.send(PPUThreadEventInfo{event, thread});
}

void Process::destroySpuThread(SPUThread* thread) {
    boost::unique_lock<boost::mutex> lock(_spuThreadMutex);
    auto it = std::find_if(begin(_spuThreads),
                           end(_spuThreads),
                           [=](auto& th) { return th.get() == thread; });
    assert(it != end(_spuThreads));
    _spuThreads.erase(it);
}

std::vector<PPUThread*> Process::dbgPPUThreads() {
    std::vector<PPUThread*> vec;
    for (auto& th : _threads)
        vec.push_back(th.get());
    return vec;
}

std::vector<SPUThread*> Process::dbgSPUThreads() {
    std::vector<SPUThread*> vec;
    for (auto& th : _spuThreads)
        vec.push_back(th.get());
    return vec;
}
