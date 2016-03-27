#include "PPUThread.h"
#include "../Process.h"
#include "../MainMemory.h"
#include "ppu_dasm.h"
#include <boost/log/trivial.hpp>

#ifdef DEBUG
#define dbgpause(value) _dbgPaused = value
#else
#define dbgpause(value)
#endif

PPUThread::PPUThread(Process* proc,
                     std::function<void(PPUThread*, PPUThreadEvent)> eventHandler,
                     bool primaryThread)
    : _proc(proc),
      _mm(proc->mm()),
      _eventHandler(eventHandler),
      _init(false),
      _isStackInfoSet(false),
      _threadFinishedGracefully(primaryThread),
      _priority(1000) {
          
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
    for (;;) {
#ifdef DEBUG
        if (_singleStep) {
            _eventHandler(this, PPUThreadEvent::SingleStepBreakpoint);
            dbgpause(true);
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
            _mm->readMemory(cia, &instr, sizeof instr);
            setNIP(cia + sizeof instr);
            ppu_dasm<DasmMode::Emulate>(&instr, cia, this);
        } catch (BreakpointException& e) {
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Breakpoint);
            dbgpause(true);
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
            break;
        } catch (ThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _threadFinishedGracefully = true;
            break;
        } catch (std::exception& e) {
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("thread exception: %s", e.what());
            setNIP(cia);
            _eventHandler(this, PPUThreadEvent::Failure);
            break;
        }
    }
}

void PPUThread::loop() {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("thread loop started");
    _eventHandler(this, PPUThreadEvent::Started);
    dbgpause(true);
    
    innerLoop();
    
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("thread loop finished (%s)", 
        _threadFinishedGracefully ? "gracefully" : "with a failure"
    );
    _eventHandler(this, PPUThreadEvent::Finished);
}

MainMemory* PPUThread::mm() {
    return _mm;
}

Process* PPUThread::proc() {
    return _proc;
}

PPUThread::PPUThread(MainMemory* mm) : _mm(mm) {}

#ifdef DEBUG
void PPUThread::singleStepBreakpoint() {
    _singleStep = true;
}

void PPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}
#endif

void PPUThread::run() {
    if (!_init) {
        _thread = boost::thread([=] { loop(); });
        _init = true;
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
    if (_threadFinishedGracefully)
        return _exitCode;
    if (unique)
        throw std::runtime_error("joining failed thread");
    _eventHandler(this, PPUThreadEvent::Joined);
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