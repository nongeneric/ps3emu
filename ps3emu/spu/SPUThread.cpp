#include "SPUThread.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../log.h"
#include "SPUDasm.h"
#include <stdio.h>
#include <signal.h>

void SPUThread::run() {
    _thread = boost::thread([=] { loop(); });
}

void SPUThread::loop() {
    LOG << ssnprintf("spu thread loop started");
    _eventHandler(this, SPUThreadEvent::Started);
    _dbgPaused = true;
    log_set_thread_name(ssnprintf("spu %d", _id));
    
#ifdef DEBUG
    auto f = fopen(ssnprintf("/tmp/spu_ls_dump_%x.bin", getNip()).c_str(), "w");
    assert(f);
    fwrite(ptr(0), LocalStorageSize, 1, f);
    fclose(f);
#endif

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    
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
            _cause = SPUThreadExitCause::Exit;
            break;
        } catch (SPUThreadInterruptException& e) {
            _interruptHandler();
        } catch (std::exception& e) {
            LOG << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    LOG << ssnprintf("spu thread loop finished");
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
      _exitCode(0),
      _cause(SPUThreadExitCause::StillRunning) {
    for (auto& r : _rs) {
        r.dw<0>() = 0;
        r.dw<1>() = 0;
    }
    std::fill(std::begin(_ch), std::end(_ch), 0);
    std::fill(std::begin(_ls), std::end(_ls), 0);
    _ch[MFC_WrTagMask] = -1;
    _status = 0b11000; // BTHSM
}

SPUThreadExitInfo SPUThread::tryJoin() {
    if (_thread.try_join_for( boost::chrono::milliseconds(0) )) {
        return SPUThreadExitInfo{ SPUThreadExitCause::StillRunning, 0 };
    }
    return SPUThreadExitInfo{_cause, _exitCode};
}

SPUThreadExitInfo SPUThread::join() {
    _thread.join();
    return SPUThreadExitInfo{_cause, _exitCode};
}

uint32_t SPUThread::getElfSource() {
    return _elfSource;
}

void SPUThread::setElfSource(uint32_t src) {
    LOG << ssnprintf("setting elf source to #%x", src);
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

#define MFC_GETLLAR_CMD      0x00d0
#define MFC_PUTLLC_CMD       0x00b4
#define MFC_PUTLLUC_CMD      0x00b0
#define MFC_PUTQLLUC_CMD     0x00b8

void SPUThread::command(uint32_t word) {
    union {
        uint32_t val;
        BitField<0, 8> tid;
        BitField<8, 16> rid;
        BitField<16, 32> opcode;
    } cmd = { word };
    auto eal = ch(MFC_EAL);
    auto lsa = ptr(ch(MFC_LSA));
    auto size = ch(MFC_Size);
    auto opcode = cmd.opcode.u();
    switch (opcode) {
        case MFC_GETLLAR_CMD: {
            _proc->mm()->readReserve(eal, lsa, size);
            // reservation always succeeds
            ch(MFC_RdAtomicStat) |= 0b100; // G
            break;
        }
        case MFC_GET_CMD:
        case MFC_GETS_CMD:
        case MFC_GETF_CMD:
        case MFC_GETB_CMD:
        case MFC_GETFS_CMD:
        case MFC_GETBS_CMD: {
            __sync_synchronize();
            _proc->mm()->readMemory(eal, lsa, size);
            __sync_synchronize();
            break;
        }
        case MFC_PUTLLC_CMD: // TODO: handle sizes correctly
        case MFC_PUTLLUC_CMD:
        case MFC_PUTQLLUC_CMD: {
            auto stored = _proc->mm()->writeCond(eal, lsa, size);
            if (opcode == MFC_PUTLLUC_CMD) {
                if (!stored) {
                    _proc->mm()->writeMemory(eal, lsa, size);
                } else {
                    ch(MFC_RdAtomicStat) |= 0b010; // U
                }
            } else if (opcode == MFC_PUTLLC_CMD) {
                ch(MFC_RdAtomicStat) |= !stored; // S
            }
            break;
        }
        case MFC_PUT_CMD:
        case MFC_PUTS_CMD:
        case MFC_PUTR_CMD:
        case MFC_PUTF_CMD:
        case MFC_PUTB_CMD:
        case MFC_PUTFS_CMD:
        case MFC_PUTBS_CMD:
        case MFC_PUTRF_CMD:
        case MFC_PUTRB_CMD: {
            __sync_synchronize();
            _proc->mm()->writeMemory(eal, lsa, size);
            __sync_synchronize();
            break;
        }
        default: throw std::runtime_error("not implemented");
    }
}

void SPUThread::setInterruptHandler(std::function<void()> interruptHandler) {
    _interruptHandler = interruptHandler;
}

std::atomic<uint32_t>& SPUThread::getStatus() {
    return _status;
}

void SPUThread::setId(uint64_t id){
    _id = id;
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

void SPUThread::cancel() {
    pthread_cancel(_thread.native_handle());
}
