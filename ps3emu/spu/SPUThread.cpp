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
    _dbgPaused = true;
    
    for (;;) {
        if (_singleStep) {
            _eventHandler(this, SPUThreadEvent::Breakpoint);
            _singleStep = false;
            _dbgPaused = true;
        }
        while (_dbgPaused) {
            ums_sleep(100);
        }
        
        uint32_t cia;
        try {
            cia = getNip();
            setNip(cia + 4);
            SPUDasm<DasmMode::Emulate>(ptr(cia), cia, this);
        } catch (BreakpointException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Breakpoint);
            _dbgPaused = true;
        } catch (IllegalInstructionException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::InvalidInstruction);
            break;
        } catch (SPUThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _cause = e.cause();
            break;
        } catch (std::exception& e) {
            BOOST_LOG_TRIVIAL(fatal) << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("spu thread loop finished");
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
      _dbgPaused(true),
      _singleStep(false),
      _exitCode(0) {
    memset(_rs, 0, sizeof(_rs));
}

SPUThreadExitInfo SPUThread::join() {
    _thread.join();
    return SPUThreadExitInfo{_cause, _exitCode};
}

uint32_t SPUThread::getElfSource() {
    return _elfSource;
}

void SPUThread::setElfSource(uint32_t src) {
    _elfSource = src;
}