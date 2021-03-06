#include "Process.h"

#include "Config.h"
#include "ContentManager.h"
#include "ELFLoader.h"
#include "InternalMemoryManager.h"
#include "MainMemory.h"
#include "log.h"
#include "ppu/InterruptPPUThread.h"
#include "ps3emu/HeapMemoryAlloc.h"
#include "ps3emu/build-config.h"
#include "ps3emu/libs/audio/libaudio.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/profiler.h"
#include "ps3emu/utils/debug.h"
#include "rsx/Rsx.h"
#include "state.h"
#include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <boost/align.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread/locks.hpp>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <set>

namespace fs = std::filesystem;

ELFLoader* Process::elfLoader() {
    return _elf.get();
}

std::optional<fdescr> Process::findExport(ELFLoader* prx, uint32_t eid, ps3_uintptr_t* fdescrva) {
    prx_export_t* exports;
    int count;
    std::tie(exports, count) = prx->exports();
    for (auto i = 0; i < count; ++i) {
        auto fnids = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].fnid_table, 4 * exports[i].functions);
        auto stubs = (big_uint32_t*)g_state.mm->getMemoryPointer(exports[i].stub_table, 4 * exports[i].functions);
        for (auto j = 0; j < exports[i].functions; ++j) {
            if (fnids[j] == eid) {
                if (fdescrva) {
                    *fdescrva = stubs[j];
                }
                fdescr descr;
                g_state.mm->readMemory(stubs[j], &descr, sizeof(descr));
                return descr;
            }
        }
    }
    return {};
}

uint32_t findExportedModuleFunction(uint32_t imageBase, const char* fname) {
    auto& segments = g_state.proc->getSegments();
    auto segment = boost::find_if(segments, [=](auto& s) { return s.va == imageBase; });
    if (segment == end(segments)) {
        WARNING(libs) << sformat("module {:x} not found", imageBase);
        return 0;
    }
    auto elfName = segment->elf->shortName();
    for (auto name : {"libgcm_sys.sprx.elf",
                      "libsysutil.sprx.elf",
                      "libsysutil_np_trophy.sprx.elf",
                      "libsysutil_game.sprx.elf",
                      "libsysutil_np.sprx.elf",
                      "libsysutil_np2.sprx.elf",
                      "libnetctl.sprx.elf",
                      "libnet.sprx.elf",
                      "libgem.sprx.elf",
                      "libsysutil_userinfo.sprx.elf",
                      "libcamera.sprx.elf",
                      "libresc.sprx.elf"}) {
        if (name == elfName) {
            INFO(libs) << sformat("ignoring function {} for module {}", fname, elfName);
            return 0;
        }
    }
    ps3_uintptr_t fdescrva;
    if (g_state.proc->findExport(segment->elf.get(), calcEid(fname), &fdescrva))
        return fdescrva;
    return 0;
}

int32_t executeExportedFunction(uint32_t imageBase,
                                size_t args,
                                ps3_uintptr_t argp,
                                ps3_uintptr_t modres, // big_int32_t*
                                PPUThread* thread,
                                const char* name,
                                boost::context::continuation* sink) {
    auto& segments = g_state.proc->getSegments();
    auto segment = boost::find_if(segments, [=](auto& s) { return s.va == imageBase; });
    assert(segment != end(segments));
    auto elfName = segment->elf->shortName();
    for (auto elf : { "libaudio.sprx.elf", "libsysutil.sprx.elf" }) {
        if (elf == elfName) {
            INFO(libs) << sformat("ignoring function {} for module {}", name, elfName);
            return CELL_OK;
        }
    }
    auto func = g_state.proc->findExport(segment->elf.get(), calcEid(name));
    if (!func) {
        INFO(libs) << sformat("module {:08x} has no {} function", imageBase, name);
        return CELL_OK;
    }
    INFO(libs) << sformat("calling {} for module {}", name, elfName);
    //thread->setGPR(3, args);
    //thread->setGPR(4, argp);
    return thread->ps3call(*func, nullptr, 0, sink);
}

void Process::insertSegment(ModuleSegment segment) {
    _segments.push_back(segment);
    std::inplace_merge(begin(_segments),
                       --end(_segments),
                       end(_segments),
                       [&](auto& a, auto& b) { return a.va < b.va; });
}

void Process::loadPrxStore() {
    fs::path storePath = g_state.config->prxStorePath;
    loadPrx((storePath / "sys" / "external" / "liblv2.sprx.elf").string());
}

int32_t EmuInitLoadedPrxModules(PPUThread* thread, boost::context::continuation* sink) {
    uint32_t modresVa;
    g_state.memalloc->internalAlloc<4, uint32_t>(&modresVa);
    auto& segments = g_state.proc->getSegments();
    // TODO: make the recursive ps3call using continuations instead of the for loop
    // when prx store contains modules other than lv2
    // (lv2 loads and starts all other modules)
    for (auto i = 1u; i < segments.size(); ++i) {
        auto& s = segments[i];
        if (s.index != 0)
            continue;
        thread->setGPR(11, g_state.elf->entryPoint()); // module_start of lv2 branches to this fdescr
        executeExportedFunction(s.va, 0, 0, modresVa, thread, "module_start", sink);
    }
    return thread->getGPR(3);
}

void Process::init(std::string elfPath, std::vector<std::string> args) {
    if (!glfwInit()) {
        throw std::runtime_error("glfw initialization failed");
    }

    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        ERROR(libs) << sformat("SDL initialization failed: {}", SDL_GetError());
        exit(1);
    }

    initAudio();

    g_profiler_process_domain = __itt_domain_create("proc");
    _internalMemoryManager.reset(new InternalMemoryManager(EmuInternalArea,
                                                           EmuInternalAreaSize,
                                                           "internal alloc"));
    _heapMemoryManager.reset(new HeapMemoryAlloc());
    _stackBlocks.reset(new InternalMemoryManager(StackArea,
                                                 StackAreaSize,
                                                 "stack allock"));
    g_state.memalloc = _internalMemoryManager.get();
    g_state.heapalloc = _heapMemoryManager.get();

    _threads.emplace_back(std::make_unique<PPUThread>(
        [this](auto t, auto e, auto p) { ppuThreadEventHandler(t, e, p); }, true));
    _rsx.reset(new Rsx());
    _elf.reset(new ELFLoader());
    _elf->load(elfPath);
    _elf->map([&](auto va, auto size, auto index) {
        insertSegment({_elf, index, va, size});
    }, 0, g_state.config->x86Paths, &_rewriterStore, false);
    if (!g_state.config->sysPrxInfos.empty()) {
        auto& infos = g_state.config->sysPrxInfos;
        auto lv2 = std::min_element(begin(infos), end(infos), [&](auto& a, auto& b) {
            return a.imageBase < b.imageBase;
        });
        HARD_ASSERT(lv2 != end(infos));
        HARD_ASSERT(_segments.back().va + _segments.back().size < lv2->imageBase);
    }
    _prxs.push_back(_elf);
    loadPrxStore();
    _contentManager.reset(new ContentManager());
    _contentManager->setElfPath(elfPath);
    _threadInitInfo.reset(new ThreadInitInfo());
    *_threadInitInfo = _elf->getThreadInitInfo();
    auto thread = _threads.back().get();
    thread->setId(g_ppuThreadBaseId, "main");

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
    thread->setGPR(7, thread->getId());
    _threadIds.create(std::move(thread));
}

std::optional<SysPrxInfo> getSysPrxInfo(std::string prxPath) {
    auto& infos = g_state.config->sysPrxInfos;
    auto it = std::find_if(begin(infos), end(infos), [&](auto& info) {
        return info.name == fs::path(prxPath).filename().string();
    });
    if (it == end(infos))
        return {};
    return *it;
}

uint32_t Process::loadPrx(std::string path) {
    auto prx = std::make_shared<ELFLoader>();
    _prxs.push_back(prx);
    prx->load(path);
    assert(!_segments.empty());
    auto prxInfo = getSysPrxInfo(path);
    uint32_t imageBase;
    if (prxInfo) {
        imageBase = prxInfo->imageBase;
    } else {
        auto& lastSegment = _segments.back();
        uint32_t available = lastSegment.va + lastSegment.size;
        if (!g_state.config->sysPrxInfos.empty()) {
            auto& lastInfo = g_state.config->sysPrxInfos.back();
            available = std::max(available, lastInfo.imageBase + lastInfo.size);
        }
        imageBase = boost::alignment::align_up(available, 1 << 10);
    }
    std::vector<std::string> x86paths;
    if (prxInfo) {
        auto dir = fs::path(path).parent_path().string();
        auto filename = fs::path(path).filename().string();
        auto x86path = sformat("{}/{}/{}.so", dir, g_buildName, filename);
        auto x86spuPath = sformat("{}/{}/{}.spu.so", dir, g_buildName, filename);

        INFO(libs) << sformat("looking for {}", x86path);
        if (prxInfo->loadx86 && fs::exists(x86path)) {
            x86paths.push_back(x86path);
        }

        INFO(libs) << sformat("looking for {}", x86spuPath);
        if (prxInfo->loadx86spu && fs::exists(x86spuPath)) {
            x86paths.push_back(x86spuPath);
        }
    }
    auto stolen = prx->map([&](auto va, auto size, auto index) {
        insertSegment({prx, index, va, size});
    }, imageBase, x86paths, &_rewriterStore, false);
    std::copy(begin(stolen), end(stolen), std::back_inserter(_stolenInfos));
    for (auto p : _prxs) {
        p->link(_prxs);
    }
    if (g_state.th) {
        g_state.th->raiseModuleLoaded(imageBase);
    }
    return imageBase;
}

void Process::dbgPause(bool pause) {
#ifdef DEBUGPAUSE
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

    for (;;) {
        {
            boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
            boost::lock_guard<boost::recursive_mutex> __(_spuThreadMutex);
            if (_spuThreads.empty() && _processFinished) {
                // there might be a dangling spu_printf thread or similar
                bool dangling = false;
                for (auto& t : _threads) {
                    WARNING(libs) << sformat("a dangling thread at ProcessFinished {}", t->getName());
                    dangling = true;
                }
                if (dangling) {
                    WARNING(libs) << "terminating process with dangling threads";
                    exit(0);
                }
                INFO(libs) << "process finishes cleanly, without dangling threads";
                return ProcessFinishedEvent{};
            }
        }

        auto removeThread = [&](PPUThread* thread) {
            boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
            auto it = std::find_if(begin(_threads), end(_threads), [=](auto& th) {
                return th.get() == thread;
            });
            assert(it != end(_threads));
            _stackBlocks->free(thread->getStackBase());
            _threadIds.destroy(thread->getId());
            _threads.erase(it);
        };

        dbgPause(false);
        auto threadEvent = _eventQueue.dequeue();
        dbgPause(true);
        threadEvent.promise->signal();

        if (_firstRun) {
            _firstRun = false;
        }

        if (auto ev = boost::get<PPUThreadEventInfo>(&threadEvent.info)) {
            switch (ev->event) {
                case PPUThreadEvent::Started: return PPUThreadStartedEvent{ev->thread};
                case PPUThreadEvent::ProcessFinished: {
                    dbgPause(false);
                    {
                        boost::lock_guard lock(_spuThreadMutex);
                        for (auto& t : _spuThreads) {
                            if (t->tryJoin(500).cause == SPUThreadExitCause::StillRunning) {
                                WARNING(spu) << sformat("a dangling SPU thread at ProcessFinished {}, suspending..", t->getName());
                                t->suspend();
                            }
                        }
                    }
                    _spuThreads.clear();
                    {
                        boost::lock_guard lock(_ppuThreadMutex);
                        removeThread(ev->thread);
                    }
                    if (_threads.size() > 1) {
                        WARNING(libs) << "dangling PPU threads at process finish";
                        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
                    }
                    _rsx->shutdown();
                    _processFinished = true;
                    break;
                }
                case PPUThreadEvent::Breakpoint: return PPUBreakpointEvent{ev->thread};
                case PPUThreadEvent::SingleStepBreakpoint:
                    return PPUSingleStepBreakpointEvent{ev->thread};
                case PPUThreadEvent::InvalidInstruction:
                    return PPUInvalidInstructionEvent{ev->thread};
                case PPUThreadEvent::MemoryAccessError:
                    return MemoryAccessErrorEvent{ev->thread};
                case PPUThreadEvent::Finished: {
                    return PPUThreadFinishedEvent{nullptr};
                }
                case PPUThreadEvent::Failure: return PPUThreadFailureEvent{ev->thread};
                case PPUThreadEvent::ModuleLoaded:
                    return PPUModuleLoadedEvent{ev->thread, std::any_cast<uint32_t>(ev->payload)};
                case PPUThreadEvent::Joined:
                    removeThread(ev->thread);
                    break;
            }
        } else if (auto ev = boost::get<SPUThreadEventInfo>(&threadEvent.info)) {
            switch (ev->event) {
                case SPUThreadEvent::Breakpoint: return SPUBreakpointEvent{ev->thread};
                case SPUThreadEvent::SingleStepBreakpoint:
                    return SPUSingleStepBreakpointEvent{ev->thread};
                case SPUThreadEvent::Started: return SPUThreadStartedEvent{ev->thread};
                case SPUThreadEvent::Finished: {
//                    ev->thread->join();
//                     boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
//                     auto it =
//                         std::find_if(begin(_spuThreads),
//                                      end(_spuThreads),
//                                      [=](auto& th) { return th.get() == ev->thread; });
//                     assert(it != end(_spuThreads));
//                     _spuThreads.erase(it);
//                     return SPUThreadFinishedEvent{nullptr};
                    break;
                }
                case SPUThreadEvent::InvalidInstruction:
                    return SPUInvalidInstructionEvent{ev->thread};
                case SPUThreadEvent::Failure: return SPUThreadFailureEvent{ev->thread};
            }
        } else {
            throw std::runtime_error("unknown event");
        }
    }
}

uint64_t Process::createThread(uint32_t stackSize,
                               ps3_uintptr_t entryPointDescriptorVa,
                               uint64_t arg,
                               std::string name,
                               uint32_t tls,
                               bool start) {
    boost::lock_guard<boost::recursive_mutex> _(_ppuThreadMutex);
    _threads.emplace_back(std::make_unique<PPUThread>(
        [this](auto t, auto e, auto p) { ppuThreadEventHandler(t, e, p); }, false));
    auto t = _threads.back().get();
    initNewThread(t, entryPointDescriptorVa, stackSize, tls);
    t->setGPR(3, arg);
    auto id = _threadIds.create(std::move(t));
    t->setId(id, name);
    t->setGPR(7, id);
    INFO(libs) << sformat("thread {:x} created", id);
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
        [this](auto t, auto e, auto p) { this->ppuThreadEventHandler(t, e, p); });
    t->setArg(arg);
    t->setEntry(entryPointDescriptorVa);
    initNewThread(t, entryPointDescriptorVa, stackSize, tls);
    auto id = _threadIds.create(std::move(t));
    INFO(libs) << sformat("interrupt thread {} created", id);
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
    assert(stack < StackArea + StackAreaSize);
    assert(stack > StackArea);
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
    thread->setGPR(9, _threadInitInfo->tlsFileSize); // TODO: fix
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
    _mainMemory->setMemory(vaArgs, 0, (args.size() + 1) * 8);
    auto len = 0;
    for (auto arg : args) {
        auto vaPtr = vaArgs + (args.size() + 1) * 8 + len;
        _mainMemory->writeMemory(vaPtr, arg.data(), arg.size() + 1);
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
        std::make_shared<SPUThread>(name, [this](auto t, auto e) {
            auto oneTime = std::make_shared<OneTimeEvent>();
            _eventQueue.enqueue(ThreadEvent{SPUThreadEventInfo{e, t}, oneTime});
            oneTime->wait();
        }));
    auto t = _spuThreads.back();
    auto id = _spuThreadIds.create(std::move(t));
    _spuThreadIds.get(id)->setId(id);
    INFO(libs) << sformat("spu thread {} created", id);
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

void Process::ppuThreadEventHandler(PPUThread* thread, PPUThreadEvent event, std::any payload) {
    auto oneTime = std::make_shared<OneTimeEvent>();
    _eventQueue.enqueue(ThreadEvent{PPUThreadEventInfo{event, thread, payload}, oneTime});
    oneTime->wait();
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

Process::~Process() = default;

Process::Process() : _eventQueue(1) {
    g_state.proc = this;
    _mainMemory.reset(new MainMemory());
    g_state.mm = _mainMemory.get();
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
    return boost::chrono::duration_cast<boost::chrono::nanoseconds>(diff);
}

std::vector<ModuleSegment>& Process::getSegments() {
    return _segments;
}

void Process::unloadSegment(uint32_t va) {
    auto it = std::find_if(begin(_segments), end(_segments), [&](auto& s) {
        return s.va == va;
    });
    if (it == end(_segments))
        throw std::runtime_error("unloading non-existent segment");
    _segments.erase(it);
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
