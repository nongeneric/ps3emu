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
            _eventHandler(this, SPUThreadEvent::SingleStepBreakpoint);
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
        } catch (StopSignalException& e) {
            _status |= 0b10;
            break;
        } catch (SPUThreadInterruptException& e) {
            _interruptHandler();
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
    _status = 0b11000; // BTHSM
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

#define MFC_PUT_CMD          0x0020
#define MFC_PUTS_CMD         0x0028   /*  PU Only */
#define MFC_PUTR_CMD         0x0030
#define MFC_PUTF_CMD         0x0022
#define MFC_PUTB_CMD         0x0021
#define MFC_PUTFS_CMD        0x002A   /*  PU Only */
#define MFC_PUTBS_CMD        0x0029   /*  PU Only */
#define MFC_PUTRF_CMD        0x0032
#define MFC_PUTRB_CMD        0x0031
#define MFC_PUTL_CMD         0x0024   /* SPU Only */
#define MFC_PUTRL_CMD        0x0034   /* SPU Only */
#define MFC_PUTLF_CMD        0x0026   /* SPU Only */
#define MFC_PUTLB_CMD        0x0025   /* SPU Only */
#define MFC_PUTRLF_CMD       0x0036   /* SPU Only */
#define MFC_PUTRLB_CMD       0x0035   /* SPU Only */

#define MFC_GET_CMD          0x0040
#define MFC_GETS_CMD         0x0048   /*  PU Only */
#define MFC_GETF_CMD         0x0042
#define MFC_GETB_CMD         0x0041
#define MFC_GETFS_CMD        0x004A   /*  PU Only */
#define MFC_GETBS_CMD        0x0049   /*  PU Only */
#define MFC_GETL_CMD         0x0044   /* SPU Only */
#define MFC_GETLF_CMD        0x0046   /* SPU Only */
#define MFC_GETLB_CMD        0x0045   /* SPU Only */

void SPUThread::command(uint32_t word) {
    union {
        uint32_t val;
        BitField<0, 8> tid;
        BitField<8, 16> rid;
        BitField<16, 32> opcode;
    } cmd = { word };
    switch (cmd.opcode.u()) {
        case MFC_GET_CMD: {
            auto ea = ch(MFC_EAL);
            auto ls = ptr(ch(MFC_LSA));
            auto size = ch(MFC_Size);
            _proc->mm()->readMemory(ea, ls, size);
            __sync_synchronize();
            break;
        }
        case MFC_PUT_CMD: {
            auto ea = ch(MFC_EAL);
            auto ls = ptr(ch(MFC_LSA));
            auto size = ch(MFC_Size);
            _proc->mm()->writeMemory(ea, ls, size);
            __sync_synchronize();
            break;
        }
        default: throw std::runtime_error("unknown mfc command");
    }
}

void SPUThread::setInterruptHandler(std::function<void()> interruptHandler) {
    _interruptHandler = interruptHandler;
}

std::atomic<uint32_t>& SPUThread::getStatus() {
    return _status;
}

ConcurrentFifoQueue<uint32_t>& SPUThread::getFromSpuMailbox() {
    return _fromSpuMailbox;
}

ConcurrentFifoQueue<uint32_t>& SPUThread::getFromSpuInterruptMailbox() {
    return _fromSpuInterruptMailbox;
}

ConcurrentFifoQueue<uint32_t>& SPUThread::getToSpuMailbox() {
    return _toSpuMailbox;
}