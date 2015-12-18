#include "PPUThread.h"
#include "Process.h"
#include "ppu_dasm.h"

PPUThread::PPUThread(Process* proc,
                     std::function<void(PPUThread*, PPUThreadEvent)> eventHandler)
    : _proc(proc),
      _mm(proc->mm()),
      _eventHandler(eventHandler),
      _init(false),
      _dbgPaused(false),
      _singleStep(false)
{
    for (auto& r : _GPR)
        r = 0;
    for (auto& r : _FPR)
        r = 0;
    for (auto& r : _V)
        r = 0;
}

void PPUThread::loop() {
    _eventHandler(this, PPUThreadEvent::Started);
    
    for (;;) {
        if (_singleStep) {
            _eventHandler(this, PPUThreadEvent::Breakpoint);
            _dbgPaused = true;
            _singleStep = false;
        }
        while (_dbgPaused) {
            ums_sleep(100);
        }
        
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
        } catch (IllegalInstructionException& e) {
            _eventHandler(this, PPUThreadEvent::InvalidInstruction);
            break;
        } catch (MemoryAccessException& e) {
            _eventHandler(this, PPUThreadEvent::MemoryAccessError);
            break;
        } catch (...) {
            _eventHandler(this, PPUThreadEvent::Failure);
            break;
        }
    }
    _eventHandler(this, PPUThreadEvent::Finished);
}

void PPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}

MainMemory* PPUThread::mm() {
    return _mm;
}

Process* PPUThread::proc() {
    return _proc;
}

PPUThread::PPUThread(MainMemory* mm) : _mm(mm) { }

void PPUThread::singleStepBreakpoint() {
    _singleStep = true;
}

void PPUThread::run() {
    if (!_init) {
        _thread = boost::thread([=] { loop(); });
        _init = true;
    }
}