#include "Process.h"

#include "MainMemory.h"
#include "ContentManager.h"
#include "InternalMemoryManager.h"
#include "ELFLoader.h"
#include "ppu/CallbackThread.h"

#include "ppu/InterruptPPUThread.h"
#include <boost/thread/locks.hpp>
#include <boost/range/algorithm.hpp>
#include "log.h"
#include <set>

MainMemory* Process::mm() {
    return _mainMemory.get();
}

ELFLoader* Process::elfLoader() {
    return _elf.get();
}

Rsx* Process::rsx() {
    return _rsx.get();
}

void Process::init(std::string elfPath, std::vector<std::string> args) {
    _threads.emplace_back(std::make_unique<PPUThread>(
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }, true));
    _rsx.reset(new Rsx());
    _mainMemory->setRsx(_rsx.get());
    _mainMemory->setProc(this);
    _elf.reset(new ELFLoader());
    _elf->load(elfPath);
    _elf->map(_mainMemory.get(), [&](auto va, auto size, auto index) {
        _segments.push_back({_elf, index, va, size});
    });
    _elf->link(_mainMemory.get(), {});
    _internalMemoryManager.reset(new InternalMemoryManager());
    _internalMemoryManager->setMainMemory(_mainMemory.get());
    _contentManager.reset(new ContentManager());
    _contentManager->setElfPath(elfPath);
    _threadInitInfo.reset(new ThreadInitInfo());
    *_threadInitInfo = _elf->getThreadInitInfo(_mainMemory.get());
    auto thread = _threads.back().get();
    thread->setId(0);
    initNewThread(thread,
                  _threadInitInfo->entryPointDescriptorVa,
                  _threadInitInfo->primaryStackSize);
    auto vaArgs = storeArgs(args);
    thread->setGPR(3, args.size());
    thread->setGPR(4, vaArgs);
    _threadIds.create(std::move(thread));
}

uint32_t Process::loadPrx(std::string path) {
    auto prx = std::make_shared<ELFLoader>();
    _prxs.push_back(prx);
    prx->load(path);
    assert(_segments.size());
    auto imageBase = ::align(_segments.back().va + _segments.back().size, 1 << 10);
    prx->map(_mainMemory.get(), [&](auto va, auto size, auto index) {
        _segments.push_back({prx, index, va, size});
    }, imageBase);
    _elf->relink(_mainMemory.get(), _prxs);
    return imageBase;
}

void Process::dbgPause(bool pause, bool takeMutex) {
#ifdef DEBUG
    std::unique_ptr<boost::unique_lock<boost::mutex>> _;
    if (takeMutex) {
        _.reset(new boost::unique_lock<boost::mutex>(_ppuThreadMutex));
    }
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
    
    if (_firstRun) {
        _callbackThread.reset(new CallbackThread(this));
        _firstRun = false;
    }

    if (auto ev = boost::get<PPUThreadEventInfo>(&threadEvent)) {
        switch (ev->event) {
            case PPUThreadEvent::Started: return PPUThreadStartedEvent{ev->thread};
            case PPUThreadEvent::ProcessFinished: {
                dbgPause(false, false);
                for (auto& t : _spuThreads) {
                    t->cancel();
                }
                _rsx->shutdown();
                _callbackThread->terminate();
                for (auto& t : _threads) {
                    t->join();
                }
                for (auto& t : _spuThreads) {
                    t->tryJoin();
                }
                return ProcessFinishedEvent();
            }
            case PPUThreadEvent::Finished:
                return PPUThreadFinishedEvent{ev->thread};
            case PPUThreadEvent::Breakpoint: return PPUBreakpointEvent{ev->thread};
            case PPUThreadEvent::SingleStepBreakpoint: return PPUSingleStepBreakpointEvent{ev->thread};
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
            case SPUThreadEvent::SingleStepBreakpoint:
                return SPUSingleStepBreakpointEvent{ev->thread};
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
    LOG << ssnprintf("thread %d created", id);
    t->setId(id);
    t->run();
    return id;
}

uint64_t Process::createInterruptThread(uint32_t stackSize,
                                        ps3_uintptr_t entryPointDescriptorVa,
                                        uint64_t arg) {
    boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
    auto t = new InterruptPPUThread(
        this, [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); });
    t->setArg(arg);
    t->setEntry(entryPointDescriptorVa);
    initNewThread(t, entryPointDescriptorVa, stackSize);
    auto id = _threadIds.create(std::move(t));
    LOG << ssnprintf("interrupt thread %d created", id);
    t->run();
    _threads.emplace_back(std::unique_ptr<PPUThread>(t));
    return id;
}

void Process::initNewThread(PPUThread* thread, ps3_uintptr_t entryDescriptorVa, uint32_t stackSize) {
    auto stack = _stackBlocks.alloc(stackSize);
    auto tls = _tlsBlocks.alloc(_threadInitInfo->tlsMemSize);
    
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
    
    mm->writeMemory(tls, _threadInitInfo->tlsBase, _threadInitInfo->tlsFileSize, true);
    mm->setMemory(tls + _threadInitInfo->tlsFileSize, 0, 
                  _threadInitInfo->tlsMemSize - _threadInitInfo->tlsFileSize, true);
    thread->setGPR(13, tls + 0x7000);
    thread->setFPSCR(0);
    thread->setNIP(entryDescr.va);
}

ps3_uintptr_t Process::storeArgs(std::vector<std::string> const& args) {
    uint32_t vaArgs;
    _internalMemoryManager->allocInternalMemory(&vaArgs, 10 * 1024, 128);
    std::vector<big_uint64_t> arr;
    _mainMemory->setMemory(vaArgs, 0, (args.size() + 1) * 8, true);
    auto len = 0;
    for (auto arg : args) {
        auto vaPtr = vaArgs + (args.size() + 1) * 8 + len;
        _mainMemory->writeMemory(vaPtr, arg.data(), arg.size() + 1, true);
        arr.push_back(vaPtr);
        len += arg.size() + 1;
    }
    arr.push_back(0);
    _mainMemory->writeMemory(vaArgs, arr.data(), arr.size() * 8);
    return vaArgs;
}

PPUThread* Process::getThread(uint64_t id) {
    boost::unique_lock<boost::mutex> _(_ppuThreadMutex);
    return _threadIds.get(id);
}

ContentManager* Process::contentManager() {
    return _contentManager.get();
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
    t->setId(id);
    LOG << ssnprintf("spu thread %d created", id);
    return id;
}

SPUThread* Process::getSpuThread(uint32_t id) {
    boost::unique_lock<boost::mutex> _(_spuThreadMutex);
    return _spuThreadIds.get(id);
}

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

InternalMemoryManager* Process::internalMemoryManager() {
    return _internalMemoryManager.get();
}

CallbackThread* Process::getCallbackThread() {
    return _callbackThread.get();
}

Process::~Process() = default;

Process::Process() {
    _mainMemory.reset(new MainMemory());
    _systemStart = boost::chrono::high_resolution_clock::now();
}

uint64_t Process::getFrequency() {
    return 79800000;
}

uint64_t Process::getTimeBase() {
    auto us = getTimeBaseMicroseconds();
    return (us * getFrequency() / 1000000).count();
}

boost::chrono::microseconds Process::getTimeBaseMicroseconds() {
    return boost::chrono::duration_cast<boost::chrono::microseconds>(
        getTimeBaseNanoseconds());
}

boost::chrono::nanoseconds Process::getTimeBaseNanoseconds() {
    auto now = boost::chrono::high_resolution_clock::now();
    auto diff = now - _systemStart;
    return boost::chrono::duration_cast<boost::chrono::microseconds>(diff);
}

std::vector<ModuleSegment>& Process::getSegments() {
    return _segments;
}
