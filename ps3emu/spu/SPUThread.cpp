#include "SPUThread.h"

#include "ps3emu/Process.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/log.h"
#include "ps3emu/libs/sync/event_flag.h"
#include "ps3emu/state.h"
#include "ps3emu/execmap/ExecutionMapCollection.h"
#include "ps3emu/RewriterUtils.h"
#include "ps3emu/spu/SPUGroupManager.h"
#include "ps3emu/AffinityManager.h"
#include "SPUDasm.h"
#include <boost/range/algorithm.hpp>
#include <stdio.h>
#include <xmmintrin.h>

void SPUThread::run(bool suspended) {
    assert(!_needsJoin);
    OneTimeEvent event;
    _thread = boost::thread([&] { loop(&event); });
    assignAffinity(_thread.native_handle(), AffinityGroup::SPUEmu);
    _suspended = suspended;
    event.wait();
    _needsJoin = true;
}

#define STOP_TYPE_MASK 0x3f00
#define STOP_TYPE_TERMINATE 0x0000
#define STOP_TYPE_MISC 0x0100
#define STOP_TYPE_SHARED_MUTEX 0x0200
#define STOP_TYPE_STOP_CALL 0x0400
#define STOP_TYPE_SYSTEM 0x1000
#define STOP_TYPE_RESERVE 0x2000

void suspend_handler(int num, siginfo_t* info, void*) {
    assert(num == SIGUSR1);
    assert(info->si_value.sival_ptr);
    auto th = (SPUThread*)info->si_value.sival_ptr;
    th->waitSuspended();
}

void SPUThread::loop(OneTimeEvent* event) {
    g_state.sth = this;
    g_state.granule = &_granule;
    _granule.dbgName = ssnprintf("spu_%d", _id);
    log_set_thread_name(_granule.dbgName);
    INFO(spu) << ssnprintf("spu thread loop started");

    struct sigaction s;
    s.sa_handler = nullptr;
    s.sa_sigaction = suspend_handler;
    sigemptyset(&s.sa_mask);
    s.sa_flags = SA_SIGINFO;
    s.sa_restorer = nullptr;
    sigaction(SIGUSR1, &s, nullptr);
    
    
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    set_mxcsr_for_spu();
    _pthread = pthread_self();
    
    event->signal();
    
    {
        auto disable = DisableSuspend(this);
        _eventHandler(this, SPUThreadEvent::Started);
    }

    waitSuspended();
    
//     auto traceFilePath = ssnprintf("/tmp/spu_trace_%s", _name);
//     auto tf = fopen(traceFilePath.c_str(), "w");
//     bool trace = false;
//     int instrTraced = 0;
    
    for (;;) {
#ifdef DEBUGPAUSE
        if (_singleStep) {
            _eventHandler(this, SPUThreadEvent::SingleStepBreakpoint);
        }
        
        while (_dbgPaused) {
            ums_sleep(100);
        }
#endif
        uint32_t cia;
        try {
            cia = getNip();

//             if (cia == 0x1cfc)
//                 trace = true;
//             if (cia == 0x1c44)
//                 trace = false;
//            
//             if (trace)
//             {
//                 instrTraced++;
//                 if ((instrTraced % 10000) == 0) {
//                     fclose(tf);
//                     tf = fopen(traceFilePath.c_str(), "w");
//                     instrTraced = 0;
//                 }
//                 
//                 fprintf(tf, "pc:%08x;", cia);
//                 for (auto i = 0u; i < 128; ++i) {
//                     auto v = r(i);
//                     fprintf(tf, "r%03d:%08x%08x%08x%08x;", i, 
//                             v.w<0>(),
//                             v.w<1>(),
//                             v.w<2>(),
//                             v.w<3>());
//                 }
//                 fputs("\n", tf);
//                 fflush(tf);
//             }

            uint32_t instr = *(uint32_t*)ptr(cia);
            uint32_t segment, label;
            if (dasm_bb_call(SPU_BB_CALL_OPCODE, __builtin_bswap32(instr), segment, label)) {
                g_state.proc->bbcallSpu(segment, label, cia, this);
            } else {  
                setNip(cia + 4);
                SPUDasm<DasmMode::Emulate>(&instr, cia, this);
            }
        } catch (BreakpointException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Breakpoint);
        } catch (IllegalInstructionException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::InvalidInstruction);
            break;
        } catch (SPUThreadFinishedException& e) {
            auto disable = DisableSuspend(this, true);
            _exitCode = e.errorCode();
            _cause = e.cause();
            break;
        } catch (StopSignalException& e) {
            auto disable = DisableSuspend(this, true);
            if (e.type() == SYS_SPU_THREAD_STOP_RECEIVE_EVENT) {
                handleReceiveEvent();
            } else if (e.type() == SYS_SPU_THREAD_STOP_GROUP_EXIT ||
                       e.type() == SYS_SPU_THREAD_STOP_THREAD_EXIT) {
                SPU_Status_SetStopCode(_channels.spuStatus(), e.type());
                _channels.spuStatus() |= SPU_Status_P;
                _exitCode = _channels.mmio_read(SPU_Out_MBox);
                _cause = e.type() == SYS_SPU_THREAD_STOP_GROUP_EXIT
                             ? SPUThreadExitCause::GroupExit
                             : SPUThreadExitCause::Exit;
                g_state.spuGroupManager->notifyThreadStopped(this, _cause);
                break;
            } else if (e.type() == STOP_TYPE_RESERVE) { // raw spu
                break;
            } else {
                // not implemented
                _eventHandler(this, SPUThreadEvent::Breakpoint);
                break;
            }
        } catch (SPUThreadInterruptException& e) {
            auto disable = DisableSuspend(this);
            if (_interruptHandler && (_interruptHandler->mask2 & _channels.interrupt())) {
                _channels.silently_write_interrupt_mbox(e.imboxValue());
                _interruptHandler->handler();
            } else {
                handleInterrupt(e.imboxValue());
            }
        } catch (InfiniteLoopException& e) {
            auto disable = DisableSuspend(this, true);
            SPU_Status_SetStopCode(_channels.spuStatus(), SYS_SPU_THREAD_STOP_GROUP_EXIT);
            _cause = SPUThreadExitCause::Exit;
            g_state.spuGroupManager->notifyThreadStopped(this, _cause);
            break;
        } catch (std::exception& e) {
            INFO(spu) << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    auto disable = DisableSuspend(this, true);
    WARNING(spu) << ssnprintf("spu thread loop finished, cause %s", to_string(_cause));
    _eventHandler(this, SPUThreadEvent::Finished);
}

#ifdef DEBUGPAUSE
void SPUThread::singleStepBreakpoint(bool value) {
    _singleStep = value;
}

void SPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}

bool SPUThread::dbgIsPaused() {
    return _dbgPaused;
}
#endif

SPUThread::SPUThread(std::string name,
                     std::function<void(SPUThread*, SPUThreadEvent)> eventHandler)
    : _name(name),
      _channels(g_state.mm, this, this),
      _eventHandler(eventHandler),
      _dbgPaused(false),
      _singleStep(false),
      _exitCode(0),
      _cause(SPUThreadExitCause::StillRunning),
      _hasStarted(false) {
    for (auto& r : _rs) {
        r.set_dw(0, 0);
        r.set_dw(1, 0);
    }
    std::fill(std::begin(_ls), std::end(_ls), 0);
    _channels.interrupt() = INT_Mask_class2_B | INT_Mask_class2_T;
}

SPUThreadExitInfo SPUThread::tryJoin(unsigned ms) {
    if (!_thread.try_join_for( boost::chrono::milliseconds(ms) )) {
        return SPUThreadExitInfo{ SPUThreadExitCause::StillRunning, 0 };
    }
    _needsJoin = false;
    return SPUThreadExitInfo{_cause, _exitCode};
}

SPUThreadExitInfo SPUThread::join() {
    _thread.join();
    _needsJoin = false;
    return SPUThreadExitInfo{_cause, _exitCode};
}

uint32_t SPUThread::getElfSource() {
    return _elfSource;
}

void SPUThread::setElfSource(uint32_t src) {
    INFO(spu) << ssnprintf("setting elf source to #%x", src);
    _elfSource = src;
}

#define SYS_SPU_THREAD_EVENT_USER 0x1
#define SYS_SPU_THREAD_EVENT_DMA  0x2
#define SYS_SPU_THREAD_EVENT_USER_KEY 0xFFFFFFFF53505501ULL
#define SYS_SPU_THREAD_EVENT_DMA_KEY  0xFFFFFFFF53505502ULL

void SPUThread::setInterruptHandler(uint32_t mask2, std::function<void()> interruptHandler) {
    _interruptHandler = InterruptThreadInfo{mask2, interruptHandler};
}

void SPUThread::setId(uint64_t id) {
    _id = id;
}

void SPUThread::cancel() {
    pthread_cancel(_thread.native_handle());
}

enum SpuInterruptOperation {
    SpuInterruptOperation_SendEvent = 0,
    SpuInterruptOperation_ThrowEvent = 1,
    SpuInterruptOperation_WriteInterruptMbox = 2,
    SpuInterruptOperation_SetFlag = 3
};

void SPUThread::handleInterrupt(uint32_t interruptValue) {
    auto spupData0 = interruptValue;
    auto op_port = (spupData0 >> 24) & 0xff;
    auto data0 = spupData0 & 0xffffff;
    auto port = op_port & 0x3f;
    auto op = op_port >> 6;
    
    INFO(spu) << ssnprintf("%s",
        op == SpuInterruptOperation_SendEvent ? "SpuInterruptOperation_SendEvent" :
        op == SpuInterruptOperation_ThrowEvent ? "SpuInterruptOperation_ThrowEvent" :
        op == SpuInterruptOperation_WriteInterruptMbox ? "SpuInterruptOperation_WriteInterruptMbox" :
        "SpuInterruptOperation_SetFlag"
    );
    
    if (op == SpuInterruptOperation_WriteInterruptMbox) {
        _channels.silently_write_interrupt_mbox(interruptValue);
        return;
    }
    
    auto data1 = _channels.mmio_read(SPU_Out_MBox);
    
    if (op == SpuInterruptOperation_SetFlag) {
        auto flag_id = data1;
        uint64_t bit = data0;
        sys_event_flag_set(flag_id, 1ull << bit);
        return;
    }
    
    // throw or send
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    auto info = boost::find_if(_eventQueuesToPPU, [=](auto& i) {
        return i.port == port;
    });
    assert(info != end(_eventQueuesToPPU));
    info->queue->send({SYS_SPU_THREAD_EVENT_USER_KEY, _id, ((uint64_t)port << 32) | data0, data1});
    
    if (op == SpuInterruptOperation_SendEvent) {
        _channels.mmio_write(SPU_In_MBox, CELL_OK);
    }
}

void SPUThread::handleReceiveEvent() {
    INFO(spu) << "receive event";
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    uint32_t port = _channels.mmio_read(SPU_Out_MBox);
    auto info = boost::find_if(_eventQueuesToSPU, [=](auto& i) {
        return i.port == port;
    });
    assert(info != end(_eventQueuesToSPU));
    auto event = info->queue->receive(0);
    _channels.mmio_write(SPU_In_MBox, CELL_OK);
    _channels.mmio_write(SPU_In_MBox, event.data1);
    _channels.mmio_write(SPU_In_MBox, event.data2);
    _channels.mmio_write(SPU_In_MBox, event.data3);
}

void SPUThread::connectQueue(std::shared_ptr<IConcurrentQueue<sys_event_t>> queue,
                                   uint32_t portNumber) {
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    _eventQueuesToPPU.push_back({portNumber, queue});
}

void SPUThread::bindQueue(std::shared_ptr<IConcurrentQueue<sys_event_t>> queue,
                                   uint32_t portNumber) {
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    _eventQueuesToSPU.push_back({portNumber, queue});
}

void SPUThread::unbindQueue(uint32_t portNumber) {
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    erase_if(_eventQueuesToSPU, [&](auto& eq) { return eq.port == portNumber; });
}

void SPUThread::disconnectQueue(uint32_t portNumber) {
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    erase_if(_eventQueuesToPPU, [&](auto& eq) { return eq.port == portNumber; });
}

bool SPUThread::isQueuePortAvailableToConnect(uint32_t portNumber) {
    boost::lock_guard<boost::mutex> lock(_eventQueuesMutex);
    auto it = boost::find_if(_eventQueuesToPPU, [=](auto& i) {
        return i.port == portNumber;
    });
    return it == end(_eventQueuesToPPU);
}

uint64_t SPUThread::getId() {
    return _id;
}

std::string SPUThread::getName() {
    return _name;
}

SPUChannels* SPUThread::channels() {
    return &_channels;
}

#if TESTS
SPUThread::SPUThread() : _channels(g_state.mm, this) {}
#endif

void set_mxcsr_for_spu() {
    auto mxcsr = _mm_getcsr();
    mxcsr |= _MM_ROUND_TOWARD_ZERO;
    mxcsr |= _MM_FLUSH_ZERO_ON;
    mxcsr |= _MM_EXCEPT_OVERFLOW;
    mxcsr |= _MM_DENORMALS_ZERO_ON;
    _mm_setcsr(mxcsr);
}

void SPUThread::suspend() {
    if (_suspended)
        return;
    _suspended = true;
    sigval val;
    val.sival_ptr = this;
    pthread_sigqueue(_pthread, SIGUSR1, val);
}

void SPUThread::resume() {
    _suspended = false;
}

bool SPUThread::suspended() {
    return _suspended;
}

void SPUThread::setGroup(ThreadGroup* group) {
    _group = group;
}

ThreadGroup* SPUThread::group() {
    return _group;
}

void SPUThread::disableSuspend() {
    _suspendEnabled = false;
}

void SPUThread::enableSuspend() {
    _suspendEnabled = true;
}

void SPUThread::waitSuspended() {
    if (!_suspendEnabled)
        return;
    for (int i = 0; _suspended; i++) {
        struct timespec t { 0, i };
        nanosleep(&t, &t);
        //sched_yield();
    }
}

DisableSuspend::DisableSuspend(SPUThread* sth, bool detach) : _detach(detach), _sth(sth) {
    if (sth) {
        sth->disableSuspend();
    }
}

DisableSuspend::~DisableSuspend() {
    if (!_detach && _sth) {
        _sth->enableSuspend();
        _sth->waitSuspended();
    }
}
