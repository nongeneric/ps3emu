#include "SPUThread.h"

#include "../Process.h"
#include "SPUDasm.h"
#include <boost/log/trivial.hpp>

void SPUThread::run() {
    _thread = boost::thread([=] { loop(); });
}

void SPUThread::loop() {
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("spu thread loop started");
    _eventHandler(this, SPUThreadEvent::Started);
    
    for (;;) {
        if (_singleStep) {
            _eventHandler(this, SPUThreadEvent::Breakpoint);
            _dbgPaused = true;
            _singleStep = false;
        }
        while (_dbgPaused) {
            ums_sleep(100);
        }
        
        uint32_t cia;
        try {
            uint32_t instr;
            cia = getNip();
            _proc->mm()->readMemory(cia, &instr, sizeof instr);
            setNip(cia + sizeof instr);
            SPUDasm<DasmMode::Emulate>(&instr, cia, this);
        } catch (BreakpointException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Breakpoint);
        } catch (IllegalInstructionException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::InvalidInstruction);
            break;
        } catch (ThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _threadFinishedGracefully = true;
            break;
        } catch (std::exception& e) {
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("thread loop finished (%s)", 
        _threadFinishedGracefully ? "gracefully" : "with a failure"
    );
    _eventHandler(this, SPUThreadEvent::Finished);
}

void SPUThread::singleStepBreakpoint() {
    _singleStep = true;
}

void SPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}

SPUThread::SPUThread(Process* proc,
                     std::string name,
                     std::function<void(SPUThread*, SPUThreadEvent)> eventHandler)
    : _name(name),
      _proc(proc),
      _eventHandler(eventHandler),
      _dbgPaused(false),
      _singleStep(false),
      _threadFinishedGracefully(false),
      _exitCode(0) {
    memset(_rs, 0, sizeof(_rs));
}