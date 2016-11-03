#include "SPUThread.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../log.h"
#include "ps3emu/libs/sync/event_flag.h"
#include "ps3emu/state.h"
#include "SPUDasm.h"
#include <stdio.h>
#include <signal.h>
#include <boost/range/algorithm.hpp>

void SPUThread::run() {
    assert(!_hasStarted);
    _hasStarted = true;
    _thread = boost::thread([=] { loop(); });
}

#define SYS_SPU_THREAD_STOP_YIELD 0x0100
#define SYS_SPU_THREAD_STOP_GROUP_EXIT 0x0101
#define SYS_SPU_THREAD_STOP_THREAD_EXIT 0x0102
#define SYS_SPU_THREAD_STOP_RECEIVE_EVENT 0x0110
#define SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT 0x0111
#define SYS_SPU_THREAD_STOP_SWITCH_SYSTEM_MODULE 0x0120
#define STOP_TYPE_MASK 0x3f00
#define STOP_TYPE_TERMINATE 0x0000
#define STOP_TYPE_MISC 0x0100
#define STOP_TYPE_SHARED_MUTEX 0x0200
#define STOP_TYPE_STOP_CALL 0x0400
#define STOP_TYPE_SYSTEM 0x1000
#define STOP_TYPE_RESERVE 0x2000

void SPUThread::loop() {
    log_set_thread_name(ssnprintf("spu_%d", _id));
    g_state.sth = this;
    INFO(spu) << ssnprintf("spu thread loop started");
    _eventHandler(this, SPUThreadEvent::Started);
    
#ifdef DEBUG
    auto f = fopen(ssnprintf("/tmp/spu_ls_dump_%x.bin", getNip()).c_str(), "w");
    assert(f);
    fwrite(ptr(0), LocalStorageSize, 1, f);
    fclose(f);
#endif

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    
    //f = fopen(ssnprintf("/tmp/spu_%d_trace", _id).c_str(), "w");
    
    for (;;) {
        if (_singleStep) {
            _eventHandler(this, SPUThreadEvent::SingleStepBreakpoint);
            _singleStep = false;
        }
        while (_dbgPaused) {
            ums_sleep(100);
        }
        
        uint32_t cia;
        try {
            cia = getNip();

//             std::string str;
//             auto instr = ptr(cia);
//             SPUDasm<DasmMode::Print>(instr, cia, &str);
//             std::string name;
//             SPUDasm<DasmMode::Name>(instr, cia, &name);
//             
//             fprintf(f, "pc:%08x;", cia);
//             for (auto i = 0u; i < 128; ++i) {
//                 auto v = r(i);
//                 fprintf(f, "r%03d:%08x%08x%08x%08x;", i, 
//                         v.w<0>(),
//                         v.w<1>(),
//                         v.w<2>(),
//                         v.w<3>());
//             }
//             fprintf(f, " #%s\n", str.c_str());
//             fflush(f);

            setNip(cia + 4);
            SPUDasm<DasmMode::Emulate>(ptr(cia), cia, this);
        } catch (BreakpointException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Breakpoint);
        } catch (IllegalInstructionException& e) {
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::InvalidInstruction);
            break;
        } catch (SPUThreadFinishedException& e) {
            _exitCode = e.errorCode();
            _cause = e.cause();
            break;
        } catch (StopSignalException& e) {
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
                if (_cause == SPUThreadExitCause::GroupExit) {
                    boost::unique_lock<boost::mutex> lock(_groupExitMutex);
                    for (auto th : _getGroupThreads()) {
                        if (th == _id)
                            continue;
                        g_state.proc->getSpuThread(th)->groupExit();
                    }
                }
                break;
            } else if (e.type() == STOP_TYPE_RESERVE) { // raw spu
                break;
            } else {
                throw std::runtime_error("not implemented");
            }
        } catch (SPUThreadInterruptException& e) {
            if (_interruptHandler && (_interruptHandler->mask2 & _channels.interrupt())) {
                _channels.silently_write_interrupt_mbox(e.imboxValue());
                _interruptHandler->handler();
            } else {
                handleInterrupt(e.imboxValue());
            }
        } catch (InfiniteLoopException& e) {
            boost::unique_lock<boost::mutex> lock(_groupExitMutex);
            _groupExitCv.wait(lock, [&] { return _groupExitPending; });
            _groupExitPending = false;
            SPU_Status_SetStopCode(_channels.spuStatus(), SYS_SPU_THREAD_STOP_GROUP_EXIT);
            //_exitCode = _channels.mmio_read(SPU_Out_MBox);
            _cause = SPUThreadExitCause::GroupExit;
            break;
        } catch (std::exception& e) {
            INFO(spu) << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    INFO(spu) << ssnprintf("spu thread loop finished, cause %s", to_string(_cause));
    _eventHandler(this, SPUThreadEvent::Finished);
    _hasStarted = false;
}

void SPUThread::singleStepBreakpoint() {
    _singleStep = true;
}

void SPUThread::dbgPause(bool val) {
    _dbgPaused = val;
}

bool SPUThread::dbgIsPaused() {
    return _dbgPaused;
}

SPUThread::SPUThread(Process* proc,
                     std::string name,
                     std::function<void(SPUThread*, SPUThreadEvent)> eventHandler)
    : _name(name),
      _proc(proc),
      _channels(g_state.mm, this),
      _eventHandler(eventHandler),
      _dbgPaused(false),
      _singleStep(false),
      _exitCode(0),
      _cause(SPUThreadExitCause::StillRunning),
      _hasStarted(false) {
    for (auto& r : _rs) {
        r.dw<0>() = 0;
        r.dw<1>() = 0;
    }
    std::fill(std::begin(_ls), std::end(_ls), 0);
    _channels.interrupt() = INT_Mask_class2_B | INT_Mask_class2_T;
}

SPUThreadExitInfo SPUThread::tryJoin(unsigned ms) {
    if (_thread.try_join_for( boost::chrono::milliseconds(ms) )) {
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

void SPUThread::groupExit() {
    boost::unique_lock<boost::mutex> lock(_groupExitMutex);
    _groupExitPending = true;
    _groupExitCv.notify_all();
}

void SPUThread::setGroup(std::function<std::vector<uint32_t>()> getThreads) {
    boost::unique_lock<boost::mutex> lock(_groupExitMutex);
    _getGroupThreads = getThreads;
}
