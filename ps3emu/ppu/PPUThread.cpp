#include "PPUThread.h"
#include "../Process.h"
#include "../MainMemory.h"
#include "../InternalMemoryManager.h"
#include "ppu_dasm.h"
#include "../log.h"
#include "../state.h"

#ifdef DEBUG
#define dbgpause(value) _dbgPaused = value
#else
#define dbgpause(value)
#endif

PPUThread::PPUThread(std::function<void(PPUThread*, PPUThreadEvent)> eventHandler,
                     bool primaryThread)
    : _eventHandler(eventHandler),
      _init(false),
      _isStackInfoSet(false),
      _threadFinishedGracefully(primaryThread),
      _priority(1000),
      _id(-1) {
          
#ifdef DEBUG
    _singleStep = false;
    _dbgPaused = false;
#endif
    for(auto& r : _GPR)
        r = 0;
    for(auto& r : _FPR)
        r = 0;
    for(auto& r : _V)
        r = 0;
}

void PPUThread::innerLoop() {
    assert(getNIP());
    g_state.th = this;
    
    for (;;) {
#ifdef DEBUG
        if (_singleStep) {
            _eventHandler(this, PPUThreadEvent::SingleStepBreakpoint);
            _singleStep = false;
        }
        
        while (_dbgPaused) {
            ums_sleep(100);
        }
#endif
        
        uint32_t cia;
        try {
            uint32_t instr;
            cia = getNIP();
            g_state.mm->readMemory(cia, &instr, sizeof instr);
            setNIP(cia + sizeof instr);
            ppu_dasm<DasmMode::Emulate>(&instr, cia, this);
        } catch (BreakpointException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Breakpoint);
        } catch (IllegalInstructionException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::InvalidInstruction);
            break;
        } catch (MemoryAccessException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::MemoryAccessError);
            break;
        } catch (ProcessFinishedException& e) {
            _eventHandler(this, PPUThreadEvent::ProcessFinished);
            return;
        } catch (ThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _threadFinishedGracefully = true;
            break;
        } catch (std::exception& e) {
            auto message = ssnprintf("thread exception: %s", e.what());
            ERROR(libs) << message;
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Failure);
            break;
        }
    }
    _eventHandler(this, PPUThreadEvent::Finished);
}

void PPUThread::loop() {
    log_set_thread_name(ssnprintf("ppu_%s_%d", _name, (unsigned)_id));
    LOG << ssnprintf("thread loop started");
    
    {
        boost::unique_lock<boost::mutex> lock(_mutexRunning);
        _running = true;
        _cvRunning.notify_all();
    }
    
    _eventHandler(this, PPUThreadEvent::Started);
    
    innerLoop();
    
    LOG << ssnprintf("thread loop finished (%s)",
        _threadFinishedGracefully ? "gracefully" : "with a failure"
    );
}

PPUThread::PPUThread() {}

#ifdef DEBUG
void PPUThread::singleStepBreakpoint() {
    _singleStep = true;
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
        _thread = boost::thread([=] { loop(); });
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
    _eventHandler(this, PPUThreadEvent::Joined);
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

void PPUThread::ps3call(uint32_t va, std::function<void()> then) {
    INFO(libs) << ssnprintf("ps3call: %x", va);
    _ps3calls.push({getNIP(), getLR(), then});
    
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
    auto top = thread->_ps3calls.top();
    thread->_ps3calls.pop();
    thread->setLR(top.lr);
    thread->setNIP(top.ret);
    top.then();
    // TODO: delete stub
    return thread->getGPR(3);
}

unsigned PPUThread::getId() {
    return _id;
}

std::string PPUThread::getName() {
    return _name;
}

void PPUThread::raiseModuleLoaded() {
    _eventHandler(this, PPUThreadEvent::ModuleLoaded);
}
