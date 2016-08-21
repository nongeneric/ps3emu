#include "SPUThread.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../log.h"
#include "ps3emu/state.h"
#include "SPUDasm.h"
#include <stdio.h>
#include <signal.h>
#include <boost/range/algorithm.hpp>

void SPUThread::run() {
    _thread = boost::thread([=] { loop(); });
}

void SPUThread::loop() {
    INFO(spu) << ssnprintf("spu thread loop started");
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
            if (e.type() == 0x110) {
                handleReceiveEvent();
            } else {
                SPU_Status_SetStopCode(_channels.spuStatus(), e.type());
                _channels.spuStatus() |= SPU_Status_P;
                _cause = SPUThreadExitCause::Exit;
                break;
            }
        } catch (SPUThreadInterruptException& e) {
            if (_interruptHandler && (_interruptHandler->mask2 & _channels.interrupt())) {
                _interruptHandler->handler();
            } else {
                handleSendEvent();
            }
        } catch (std::exception& e) {
            INFO(spu) << ssnprintf("spu thread exception: %s", e.what());
            setNip(cia);
            _eventHandler(this, SPUThreadEvent::Failure);
            break;
        }
    }
    
    INFO(spu) << ssnprintf("spu thread loop finished");
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
      _channels(g_state.mm, this),
      _eventHandler(eventHandler),
      _dbgPaused(true),
      _singleStep(false),
      _exitCode(0),
      _cause(SPUThreadExitCause::StillRunning) {
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

void SPUThread::handleSendEvent() {
    auto data1 = _channels.mmio_read(SPU_Out_MBox);
    auto spupData0 = _channels.mmio_read(SPU_Out_Intr_Mbox);
    auto port = (spupData0 >> 24) & 0xff;
    auto data0 = spupData0 & 0xffffff;
    auto info = boost::find_if(_eventQueues, [=](auto& i) {
        return i.port == port;
    });
    assert(info != end(_eventQueues));
    info->queue->send({SYS_SPU_THREAD_EVENT_USER_KEY, _id, ((uint64_t)port << 32) | data0, data1});
    _channels.mmio_write(SPU_In_MBox, 0);
}

void SPUThread::handleReceiveEvent() {
    uint32_t port = _channels.mmio_read(SPU_Out_MBox);
    auto info = boost::find_if(_eventQueues, [=](auto& i) {
        return i.port == port;
    });
    assert(info != end(_eventQueues));
    auto event = info->queue->receive(0);
    _channels.mmio_write(SPU_In_MBox, event.source);
    _channels.mmio_write(SPU_In_MBox, event.data1);
    _channels.mmio_write(SPU_In_MBox, event.data2);
    _channels.mmio_write(SPU_In_MBox, event.data3);
}

void SPUThread::connectOrBindQueue(std::shared_ptr<IConcurrentQueue<sys_event_t>> queue,
                                   uint32_t portNumber) {
    _eventQueues.push_back({portNumber, queue});
}

bool SPUThread::isAvailableQueuePort(uint8_t portNumber) {
    auto it = boost::find_if(_eventQueues, [=](auto& i) {
        return i.port == portNumber;
    });
    return it == end(_eventQueues);
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
