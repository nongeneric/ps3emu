#include "Process.h"

#include "MainMemory.h"
#include "ContentManager.h"
#include "InternalMemoryManager.h"
#include "ELFLoader.h"
#include "ppu/CallbackThread.h"
#include "state.h"

#include "ppu/InterruptPPUThread.h"
#include <boost/thread/locks.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include "log.h"
#include <set>
#include <cstdlib>

using namespace boost::filesystem;

ELFLoader* Process::elfLoader() {
    return _elf.get();
}

Rsx* Process::rsx() {
    return _rsx.get();
}

boost::optional<fdescr> findExport(MainMemory* mm, ELFLoader* prx, uint32_t eid, ps3_uintptr_t* fdescrva = nullptr) {
    prx_export_t* exports;
    int count;
    std::tie(exports, count) = prx->exports(mm);
    for (auto i = 0; i < count; ++i) {
        if (exports[i].name)
            continue;
        auto fnids = (big_uint32_t*)mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
        auto stubs = (big_uint32_t*)mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
        for (auto j = 0; j < exports[i].functions; ++j) {
            if (fnids[j] == eid) {
                if (fdescrva) {
                    *fdescrva = stubs[j];
                }
                fdescr descr;
                mm->readMemory(stubs[j], &descr, sizeof(descr));
                return descr;
            }
        }
    }
    return {};
}

uint32_t findExportedModuleFunction(uint32_t imageBase, const char* name) {
    auto& segments = g_state.proc->getSegments();
    auto segment = boost::find_if(segments, [=](auto& s) { return s.va == imageBase; });
    assert(segment != end(segments));
    auto elfName = segment->elf->shortName();
    for (auto name : {"libaudio.sprx.elf",
                      "libgcm_sys.sprx.elf",
                      "libsysutil.sprx.elf",
                      "libsysutil_np_trophy.sprx.elf",
                      "libsysutil_game.sprx.elf",
                      "libresc.sprx.elf"}) {
        if (name == elfName) {
            INFO(libs) << ssnprintf("ignoring function %s for module %s", name, elfName);
            return 0;
        }
    }
    ps3_uintptr_t fdescrva;
    if (findExport(g_state.mm, segment->elf.get(), calcEid(name), &fdescrva))
        return fdescrva;
    return 0;
}

int32_t executeExportedFunction(uint32_t imageBase,
                                size_t args,
                                ps3_uintptr_t argp,
                                ps3_uintptr_t modres, // big_int32_t*
                                PPUThread* thread,
                                const char* name) {
    auto& segments = g_state.proc->getSegments();
    auto segment = boost::find_if(segments, [=](auto& s) { return s.va == imageBase; });
    assert(segment != end(segments));
    auto elfName = segment->elf->shortName();
    for (auto elf : { "libaudio.sprx.elf", "libgcm_sys.sprx.elf", "libsysutil.sprx.elf" }) {
        if (elf == elfName) {
            INFO(libs) << ssnprintf("ignoring function %s for module %s", name, elfName);
            return CELL_OK;
        }
    }
    auto func = findExport(g_state.mm, segment->elf.get(), calcEid(name));
    if (!func) {
        INFO(libs) << ssnprintf("module %08x has no %s function", imageBase, name);
        return CELL_OK;
    }
    INFO(libs) << ssnprintf("calling %s for module %s", name, elfName);
    thread->setGPR(2, func->tocBase);
    //thread->setGPR(3, args);
    //thread->setGPR(4, argp);
    thread->ps3call(func->va,
                    [=] { /*g_state.mm->store<4>(modres, thread->getGPR(3));*/ });
    return thread->getGPR(3);
}

void Process::loadPrxStore() {
    auto storePath = g_state.content->prxStore();
    if (storePath.empty())
        return;
    for (auto f = directory_iterator(storePath); f != directory_iterator(); ++f) {
        if (is_directory(*f))
            continue;
        if (extension(*f) != ".prx")
            continue;
        loadPrx(f->path().string());
    }
}

int32_t EmuInitLoadedPrxModules(PPUThread* thread) {
    uint32_t modresVa;
    g_state.memalloc->internalAlloc<4, uint32_t>(&modresVa);
    auto& segments = g_state.proc->getSegments();
    // TODO: make the recursive ps3call using continuations instead of the for loop
    // when prx store contains modules other than lv2
    for (auto i = 1u; i < segments.size(); ++i) {
        auto& s = segments[i];
        if (s.index != 0)
            continue;
        thread->setGPR(11, g_state.elf->entryPoint());
        executeExportedFunction(s.va, 0, 0, modresVa, thread, "module_start");
    }
    return thread->getGPR(3);
}

void Process::init(std::string elfPath, std::vector<std::string> args) {
    g_state.proc = this;
    g_state.mm = _mainMemory.get();
    _threads.emplace_back(std::make_unique<PPUThread>(
        [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }, true));
    _rsx.reset(new Rsx());
    _elf.reset(new ELFLoader());
    _elf->load(elfPath);
    _elf->map(_mainMemory.get(), [&](auto va, auto size, auto index) {
        _segments.push_back({_elf, index, va, size});
    });
    _prxs.push_back(_elf);
    loadPrxStore();
    _internalMemoryManager.reset(new InternalMemoryManager(EmuInternalArea,
                                                           EmuInternalAreaSize));
    _heapMemoryManager.reset(new InternalMemoryManager(HeapArea, HeapAreaSize));
    _stackBlocks.reset(new InternalMemoryManager(StackArea, StackAreaSize));
    _contentManager.reset(new ContentManager());
    _contentManager->setElfPath(elfPath);
    _threadInitInfo.reset(new ThreadInitInfo());
    *_threadInitInfo = _elf->getThreadInitInfo(_mainMemory.get());
    auto thread = _threads.back().get();
    thread->setId(0, "main");
    
    g_state.memalloc = _internalMemoryManager.get();
    g_state.heapalloc = _heapMemoryManager.get();
    g_state.content = _contentManager.get();
    g_state.rsx = _rsx.get();
    g_state.elf = _elf.get();
    
    initPrimaryThread(thread,
                      _threadInitInfo->entryPointDescriptorVa,
                      _threadInitInfo->primaryStackSize);
    args.insert(begin(args), elfPath);
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
    auto stolen = prx->map(_mainMemory.get(), [&](auto va, auto size, auto index) {
        _segments.push_back({prx, index, va, size});
    }, imageBase);
    std::copy(begin(stolen), end(stolen), std::back_inserter(_stolenInfos));
    for (auto p : _prxs) {
        p->link(_mainMemory.get(), _prxs);
    }
    return imageBase;
}

void Process::dbgPause(bool pause) {
#ifdef DEBUG
    boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
    boost::lock_guard<boost::recursive_mutex> __(_spuThreadMutex);
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
                dbgPause(false);
                _rsx->shutdown();
                _callbackThread->terminate();
                for (auto& t : _threads) {
                    t->join();
                }
                for (auto& t : _spuThreads) {
                    if (t->tryJoin(200).cause == SPUThreadExitCause::StillRunning) {
                        t->cancel();
                    }
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
                boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
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
                               uint64_t arg,
                               std::string name,
                               uint32_t tls,
                               bool start) {
    boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
    _threads.emplace_back(std::make_unique<PPUThread>(
        [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); }, false));
    auto t = _threads.back().get();
    initNewThread(t, entryPointDescriptorVa, stackSize, tls);
    t->setGPR(3, arg);
    auto id = _threadIds.create(std::move(t));
    LOG << ssnprintf("thread %d created", id);
    t->setId(id, name);
    if (start)
        t->run();
    return id;
}

uint64_t Process::createInterruptThread(uint32_t stackSize,
                                        ps3_uintptr_t entryPointDescriptorVa,
                                        uint64_t arg,
                                        std::string name,
                                        uint32_t tls,
                                        bool start) {
    boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
    auto t = new InterruptPPUThread(
        [=](auto t, auto e) { this->ppuThreadEventHandler(t, e); });
    t->setArg(arg);
    t->setEntry(entryPointDescriptorVa);
    initNewThread(t, entryPointDescriptorVa, stackSize, tls);
    auto id = _threadIds.create(std::move(t));
    LOG << ssnprintf("interrupt thread %d created", id);
    if (start)
        t->run();
    _threads.emplace_back(std::unique_ptr<PPUThread>(t));
    return id;
}

struct InitPrxStub {
    big_uint32_t ncall_EmuInitLoadedPrxModules;
};

void Process::initPrimaryThread(PPUThread* thread, ps3_uintptr_t entryDescriptorVa, uint32_t stackSize) {
    uint32_t newEntryDescrVa;
    auto newEntryDescr = g_state.memalloc->internalAlloc<4, fdescr>(&newEntryDescrVa);
    _mainMemory->readMemory(entryDescriptorVa, newEntryDescr, sizeof(fdescr));
    
    uint32_t initPrxStubVa;
    g_state.memalloc->internalAlloc<4, InitPrxStub>(&initPrxStubVa);
    uint32_t index;
    auto entry = findNCallEntry(calcFnid("EmuInitLoadedPrxModules"), index);
    assert(entry); (void)entry;
    encodeNCall(g_state.mm, initPrxStubVa, index);
    
    thread->setLR(newEntryDescr->va);
    newEntryDescr->va = initPrxStubVa;
    
    initNewThread(thread, newEntryDescrVa, stackSize, 0x10);
}

void Process::initNewThread(PPUThread* thread,
                            ps3_uintptr_t entryDescriptorVa,
                            uint32_t stackSize,
                            uint32_t tls) {
    uint32_t stack;
    _stackBlocks->allocInternalMemory(&stack, stackSize + 0xf0, 64);
    thread->setStackInfo(stack, stackSize);
    
    // PPU_ABI-Specifications_e

    fdescr entryDescr;
    _mainMemory->readMemory(entryDescriptorVa, &entryDescr, sizeof(fdescr));
    
    thread->setGPR(1, stack + stackSize - 0xf0);
    thread->setGPR(2, entryDescr.tocBase);
    
    // undocumented:
    thread->setGPR(5, stack);
    thread->setGPR(6, 0);
    thread->setGPR(8, _threadInitInfo->tlsSegmentVa);
    thread->setGPR(9, _threadInitInfo->tlsFileSize);
    thread->setGPR(10, _threadInitInfo->tlsMemSize);
    thread->setGPR(12, DefaultMainMemoryPageSize);
    
    thread->setGPR(13, tls);
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
    boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
    return _threadIds.get(id);
}

uint32_t Process::createSpuThread(std::string name) {
    boost::lock_guard<boost::recursive_mutex> _(_spuThreadMutex);
    _spuThreads.emplace_back(
        std::make_shared<SPUThread>(this,
                                    name,
                                    [=](auto t, auto e) {
                                        _eventQueue.send(SPUThreadEventInfo{e, t});
                                    }));
    auto t = _spuThreads.back();
    auto id = _spuThreadIds.create(std::move(t));
    _spuThreadIds.get(id)->setId(id);
    LOG << ssnprintf("spu thread %d created", id);
    return id;
}

std::shared_ptr<SPUThread> Process::getSpuThread(uint32_t id) {
    boost::lock_guard<boost::recursive_mutex> _(_spuThreadMutex);
    return _spuThreadIds.get(id);
}

std::shared_ptr<SPUThread> Process::getSpuThreadBySpuNum(uint32_t spuNum) {
    boost::lock_guard<boost::recursive_mutex> _(_spuThreadMutex);
    auto it = boost::find_if(_spuThreads, [&](auto& th) {
        return th->getSpu() == spuNum;
    });
    assert(it != end(_spuThreads));
    return *it;
}

void Process::ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event) {
    _eventQueue.send(PPUThreadEventInfo{event, thread});
}

void Process::destroySpuThread(SPUThread* thread) {
    boost::lock_guard<boost::recursive_mutex> _(_spuThreadMutex);
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

std::vector<std::shared_ptr<ELFLoader>> Process::loadedModules() {
    return _prxs;
}

StolenFuncInfo Process::getStolenInfo(uintptr_t ncallIndex) {
    auto it = boost::find_if(_stolenInfos, [=](auto& info) {
        return info.ncallIndex == ncallIndex;
    });
    assert(it != end(_stolenInfos));
    return *it;
}
