#include "PPUThread.h"
#include "ps3emu/Process.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ppu_dasm.h"
#include "ppu_dasm_forms.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/BBCallMap.h"
#include "ps3emu/AffinityManager.h"
#include "ps3emu/exports/exports.h"
#include "ps3emu/exports/splicer.h"
#include "ps3emu/EmuCallbacks.h"
#include <sys/types.h>
#include <sys/syscall.h>

#include <boost/endian/conversion.hpp>

using namespace boost::endian;

#ifdef DEBUGPAUSE
#define dbgpause(value) _dbgPaused = value
#else
#define dbgpause(value)
#endif

PPUThread::PPUThread(std::function<void(PPUThread*, PPUThreadEvent, std::any)> eventHandler,
                     bool primaryThread)
    : _eventHandler(eventHandler),
      _init(false),
      _dbgPaused(false),
      _singleStep(false),
      _isStackInfoSet(false),
      _threadFinishedGracefully(primaryThread),
      _priority(1000),
      _id(-1) {

    assert(eventHandler);

    for(auto& r : _GPR)
        r = 0;
    for(auto& r : _FPR)
        r = 0;
    for(auto& r : _V)
        r.set_xmm(_mm_setzero_si128());

    _mm = g_state.mm;
}

void PPUThread::vmenter(uint32_t to) {
    auto bbcalls = g_state.bbcallMap->base();
    
    for (;;) {
        auto cia = getNIP();
        auto bbcall = bbcalls[cia / 4];
        if (bbcall) {
            uint32_t segment, label;
            bbcallmap_dasm(bbcall, segment, label);
            g_state.proc->bbcall(segment, label, this);
        } else {
            auto instr = *(big_uint32_t*)g_state.mm->getMemoryPointer(cia, 4);
#ifdef EXECMAP_ENABLED
            g_state.executionMaps->ppu.mark(cia);
#endif
            setNIP(cia + sizeof instr);
            ppu_dasm<DasmMode::Emulate>(&instr, cia, this);
        }
    }
}

void PPUThread::innerLoop() {
    assert(getNIP());

    auto bbcalls = g_state.bbcallMap->base();

#ifdef DEBUG
    uint64_t dbgCounter = 0;
    std::array<uint32_t, 1000> dbgTrace = {0};
#endif
    
    for (;;) {
#ifdef DEBUGPAUSE
        if (_singleStep) {
            _eventHandler(this, PPUThreadEvent::SingleStepBreakpoint, {});
        }
        
        while (_dbgPaused) {
            ums_sleep(100);
        }
#endif

        uint32_t cia;
        try {
            cia = getNIP();
#ifdef DEBUG
            dbgTrace[dbgCounter % dbgTrace.size()] = cia;
            dbgCounter++;
#endif
            auto bbcall = bbcalls[cia / 4];
            if (bbcall) {
                uint32_t segment, label;
                bbcallmap_dasm(bbcall, segment, label);
                g_state.proc->bbcall(segment, label, this);
            } else {
                auto instr = *(big_uint32_t*)g_state.mm->getMemoryPointer(cia, 4);
#ifdef EXECMAP_ENABLED
                g_state.executionMaps->ppu.mark(cia);
#endif
                setNIP(cia + sizeof instr);
                ppu_dasm<DasmMode::Emulate>(&instr, cia, this);
            }
        } catch (BreakpointException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Breakpoint, {});
        } catch (IllegalInstructionException& e) {
            setNIP(cia);
            ERROR(libs) << "illegal instruction";
            _eventHandler(this, PPUThreadEvent::InvalidInstruction, {});
            break;
        } catch (MemoryAccessException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::MemoryAccessError, {});
            break;
        } catch (ProcessFinishedException& e) {
            _eventHandler(this, PPUThreadEvent::ProcessFinished, {});
            return;
        } catch (ThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _threadFinishedGracefully = true;
            break;
        } catch (std::exception& e) {
            auto message = ssnprintf("thread exception: %s", e.what());
            ERROR(libs) << message;
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Failure, {});
            break;
        }
    }
    _eventHandler(this, PPUThreadEvent::Finished, {});
}

void PPUThread::loop() {
    g_state.th = this;
    g_state.granule = &_granule;
    _granule.dbgName = ssnprintf("ppu_%s_%x", _name, (unsigned)_id);
    _tid = syscall(__NR_gettid);
    log_set_thread_name(_granule.dbgName);
    INFO(libs) << ssnprintf("thread loop started");
    
    {
        boost::unique_lock<boost::mutex> lock(_mutexRunning);
        _running = true;
        _cvRunning.notify_all();
    }
    
    _eventHandler(this, PPUThreadEvent::Started, {});
    
    innerLoop();
    
    INFO(libs) << ssnprintf("thread loop finished (%s)",
        _threadFinishedGracefully ? "gracefully" : "with a failure"
    );
}

PPUThread::PPUThread() {
    _mm = g_state.mm;
}

#ifdef DEBUGPAUSE
void PPUThread::singleStepBreakpoint(bool value) {
    _singleStep = value;
}

void PPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}

bool PPUThread::dbgIsPaused() {
    return _dbgPaused;
}
#endif

void PPUThread::run() {
    boost::unique_lock<boost::mutex> lock(_mutexRunning);
    if (!_init) {
        _thread = boost::thread([this] { loop(); });
        assignAffinity(_thread.native_handle(), AffinityGroup::PPUEmu);
        _init = true;
        _cvRunning.wait(lock, [&] { return _running; });
    }
}

void PPUThread::setStackInfo(uint32_t base, uint32_t size) {
    _isStackInfoSet = true;
    _stackBase = base;
    _stackSize = size;
}

uint32_t PPUThread::getStackBase() {
    assert(_isStackInfoSet);
    return _stackBase;
}

uint32_t PPUThread::getStackSize() {
    assert(_isStackInfoSet);
    return _stackSize;
}

uint64_t PPUThread::join(bool unique) {
    _thread.join();
    _eventHandler(this, PPUThreadEvent::Joined, {});
    if (_threadFinishedGracefully)
        return _exitCode;
    if (unique)
        throw std::runtime_error("joining failed thread");
    return 0;
}

void PPUThread::setPriority(int priority) {
    _priority = priority;
}

int PPUThread::priority() {
    return _priority;
}

void PPUThread::setArg(uint64_t arg) { }

void PPUThread::yield() {
    boost::this_thread::yield();
}

void PPUThread::setId(unsigned id, std::string name) {
    _id = id;
    _name = name;
}

struct CallStub {
    uint32_t ncall;
};

uint64_t PPUThread::ps3call(fdescr const& descriptor,
                            const uint64_t* firstArg,
                            unsigned argCount,
                            boost::context::continuation* sink) {
    for (auto i = 0u; i < argCount; ++i) {
        setGPR(i + 3, firstArg[i]);
    }
    setGPR(2, descriptor.tocBase);
    if (g_state.rewriter_ncall) {
        setNIP(descriptor.va);
        vmenter(getLR());
    } else {
        ps3call_impl(descriptor.va);
        *sink = sink->resume();
    }
    return getGPR(3);
}

uint64_t PPUThread::ps3call(fdescr const& descriptor,
                            std::initializer_list<uint64_t> args,
                            boost::context::continuation* sink) {
    return ps3call(descriptor, &*begin(args), args.size(), sink);
}

void PPUThread::ps3call_impl(uint32_t va) {
    _ps3calls.push({getNIP(), getLR()});
    
    uint32_t stubVa;
    g_state.memalloc->internalAlloc<4, CallStub>(&stubVa);
    setLR(stubVa);
    setNIP(va);
    
    uint32_t ncallIndex;
    auto entry = findNCallEntry(calcFnid("ps3call_then"), ncallIndex);
    assert(entry); (void)entry;
    encodeNCall(g_state.mm, stubVa + offsetof(CallStub, ncall), ncallIndex);
}

uint64_t ps3call_then(PPUThread* thread) {
    auto stubVa = thread->getNIP();
    auto top = thread->_ps3calls.top();
    thread->_ps3calls.pop();
    thread->setLR(top.lr);
    thread->setNIP(top.ret);
    auto& cont = thread->_pscallContinuation.top();
    cont = cont.resume();
    if (!cont) { // current coroutine has exited
        thread->_pscallContinuation.pop();
    }
    g_state.memalloc->free(stubVa);
    return thread->getGPR(3);
}

unsigned PPUThread::getId() {
    return _id;
}

pthread_t PPUThread::getHostId() {
    return _thread.native_handle();
}

std::string PPUThread::getName() {
    return _name;
}

void PPUThread::raiseModuleLoaded(uint32_t imageBase) {
    _eventHandler(this, PPUThreadEvent::ModuleLoaded, imageBase);
}

unsigned PPUThread::getTid() {
    return _tid;
}

emu_void_t ps3call_tests(fdescr* simpleDescr,
                         fdescr* recursiveDescr,
                         fdescr* recursiveChildDescr,
                         PPUThread* thread,
                         boost::context::continuation* sink) {
    // allow the child patch
    g_state.bbcallMap->set(recursiveChildDescr->va, 0);

    // otherwise the patched child will never be called
    // the rewriter will just "goto" to an already rewritten child
    g_state.bbcallMap->set(recursiveDescr->va, 0);

    auto write = [=](std::string msg) {
        printf("%s\n", msg.c_str());
    };

    write(ssnprintf("stolen ps3call_tests(%x, %x, %x)",
                    simpleDescr->va,
                    recursiveDescr->va,
                    recursiveChildDescr->va));

    write("calling simple(5,7)");
    auto res = thread->ps3call(*simpleDescr, {5ull, 7ull}, sink);
    write(ssnprintf("returned %d", res));

    write(ssnprintf("[after a ps3call] stolen ps3call_tests(%x, %x, %x)",
                    simpleDescr->va,
                    recursiveDescr->va,
                    recursiveChildDescr->va));

    auto index = addNCallEntry({"stolen_recursive_child_cb", 0, [=](PPUThread* th) {
        wrap(std::function([=](uint32_t a, boost::context::continuation* sink) {
            write("calling (from recursive child) simple(10,20)");
            auto res = thread->ps3call(*simpleDescr, {10ull, 20ull}, sink);
            write(ssnprintf("simple returned %d", res));
            return a + 17ul;
        }), th);
    }});

    encodeNCall(g_state.mm, recursiveChildDescr->va, index);

    write("calling recursive(11)");
    res = thread->ps3call(*recursiveDescr, {11ull}, sink);
    write(ssnprintf("recursive returned %d", res));

    return emu_void;
}

emu_void_t slicing_tests(fdescr* singleDescr,
                         fdescr* multipleDescr,
                         fdescr* multipleRecursiveDescr,
                         PPUThread* thread) {

    auto write = [=](std::string msg) {
        msg = ssnprintf("%s\n", msg);
        g_state.callbacks->stdout(msg.c_str(), msg.size());
    };

    spliceFunction(singleDescr->va, [=] {
        write(ssnprintf("proxy single(%d, %d)", g_state.th->getGPR(3), g_state.th->getGPR(4)));
    }, [=] {
        write(ssnprintf("proxy single = %d", g_state.th->getGPR(3)));
    });

    spliceFunction(multipleDescr->va, [=] {
        write(ssnprintf("proxy multiple(%d, %d)", g_state.th->getGPR(3), g_state.th->getGPR(4)));
    }, [=] {
        write(ssnprintf("proxy multiple = %d", g_state.th->getGPR(3)));
    });

    spliceFunction(multipleRecursiveDescr->va, [=] {
        write(ssnprintf("proxy multipleRecursive(%d, %d)", g_state.th->getGPR(3), g_state.th->getGPR(4)));
    }, [=] {
        write(ssnprintf("proxy multipleRecursive = %d", g_state.th->getGPR(3)));
    });

    return emu_void;
}
