#pragma once

#include "ps3emu/libs/ConcurrentQueue.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/constants.h"
#include "ps3emu/BitField.h"
#include "ps3emu/ReservationMap.h"
#include "ps3emu/enum.h"
#include "ps3emu/utils/debug.h"
#include "ps3emu/OneTimeEvent.h"
#include "SPUChannels.h"
#include "R128.h"
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <atomic>
#include <assert.h>
#include <algorithm>
#include <stdexcept>
#include <string.h>
#include <functional>
#include <vector>
#include <optional>
#include <x86intrin.h>
#include <signal.h>

static constexpr uint32_t LSLR = 0x3ffff;

class DisableSuspend {
    bool _detach;
    SPUThread* _sth;
    
public:
    DisableSuspend(SPUThread* sth, bool detach = false);
    ~DisableSuspend();
};

class StopSignalException : public virtual std::runtime_error {
    uint32_t _type;
public:
    StopSignalException(uint32_t type) : std::runtime_error("stop signal"), _type(type) { }
    uint32_t type() {
        return _type;
    }
};

ENUM(SPUThreadExitCause,
     (Exit, 0),
     (GroupExit, 1),
     (GroupTerminate, 2),
     (StillRunning, 3))

class SPUThreadFinishedException : public virtual std::exception {
    int32_t _errorCode;
    SPUThreadExitCause _cause;

public:
    inline SPUThreadFinishedException(int32_t errorCode, SPUThreadExitCause cause)
        : _errorCode(errorCode), _cause(cause) {}

    inline int32_t errorCode() {
        return _errorCode;
    }

    inline SPUThreadExitCause cause() {
        return _cause;
    }
};

enum class SPUThreadEvent {
    Breakpoint,
    SingleStepBreakpoint,
    Started,
    Finished,
    InvalidInstruction,
    Failure
};

struct SPUThreadExitInfo {
    SPUThreadExitCause cause;
    int32_t status;
};

struct InterruptThreadInfo {
    uint32_t mask2;
    std::function<void()> handler;
};

struct EventQueueInfo {
    uint32_t port;
    std::shared_ptr<IConcurrentQueue<sys_event_t>> queue;
};

struct ThreadGroup;

class SPUThread : boost::noncopyable, public ISPUChannelsThread {
    uint32_t _nip;
    R128 _rs[128];
    R128 _fpscr;
    uint8_t _ls[LocalStorageSize];
    uint32_t _srr0;
    uint32_t _spu = -1u;
    std::string _name;
    boost::thread _thread;
    SPUChannels _channels;
    std::function<void(SPUThread*, SPUThreadEvent)> _eventHandler;

    // see the rationale for leaving these variables without an ifdef
    // in the PPUThread.cpp file
    std::atomic<bool> _dbgPaused;
    std::atomic<bool> _singleStep;

    int32_t _exitCode;
    SPUThreadExitCause _cause;
    uint32_t _elfSource;
    std::optional<InterruptThreadInfo> _interruptHandler;
    uint64_t _id;
    boost::mutex _eventQueuesMutex;
    std::vector<EventQueueInfo> _eventQueuesToPPU;
    std::vector<EventQueueInfo> _eventQueuesToSPU;
    void loop(OneTimeEvent* started);
    void handleInterrupt(uint32_t interruptValue);
    void handleReceiveEvent();
    bool _hasStarted;
    ReservationGranule _granule;
    std::atomic<bool> _suspended = false;
    boost::mutex _suspendMutex;
    boost::condition_variable _suspendCv;
    bool _needsJoin = false;
    pthread_t _pthread;
    ThreadGroup* _group;
    bool _suspendEnabled = true;
    
    friend void suspend_handler(int);

public:
    SPUThread(std::string name,
              std::function<void(SPUThread*, SPUThreadEvent)> eventHandler);
#if TESTS
    SPUThread();
#endif
    
    template <typename V>
    inline R128& r(V i) {
        return _rs[getUValue(i)];
    }
    
    inline uint8_t* ptr(uint32_t lsa) {
        EMU_ASSERT(lsa < sizeof(_ls));
        return &_ls[lsa];
    }
    
    inline void setNip(uint32_t nip) override {
        EMU_ASSERT(nip < LSLR);
        _nip = nip;
    }
    
    inline uint32_t getNip() {
        return _nip;
    }
    
    inline uint32_t getSrr0() {
        return _srr0;
    }
    
    inline void setSrr0(uint32_t val) {
        EMU_ASSERT(val < LSLR);
        _srr0 = val;
    }
    
    inline void setSpu(uint32_t num) {
        EMU_ASSERT(num != -1u && num <= 255);
        _spu = num;
    }
    
    inline R128& fpscr() {
        return _fpscr;
    }
    
    inline uint32_t getSpu() {
        EMU_ASSERT(_spu != -1u && _spu <= 255);
        return _spu;
    }
    
    void singleStepBreakpoint(bool value);
    void dbgPause(bool val);
    bool dbgIsPaused();
    void run(bool suspended = false) override;
    void cancel();
    SPUThreadExitInfo tryJoin(unsigned ms);
    SPUThreadExitInfo join();
    void setElfSource(uint32_t src);
    uint32_t getElfSource();
    void setInterruptHandler(uint32_t mask2, std::function<void()> interruptHandler);
    // TODO: removeInterruptHandler
    void setId(uint64_t id);
    uint64_t getId();
    void connectQueue(std::shared_ptr<IConcurrentQueue<sys_event_t>> queue,
                      uint32_t portNumber);
    void bindQueue(std::shared_ptr<IConcurrentQueue<sys_event_t>> queue,
                   uint32_t portNumber);
    void disconnectQueue(uint32_t portNumber);
    void unbindQueue(uint32_t portNumber);
    bool isQueuePortAvailableToConnect(uint32_t portNumber);
    std::string getName();
    SPUChannels* channels();
    void suspend();
    void resume();
    bool suspended();
    void disableSuspend();
    void enableSuspend();
    void waitSuspended();
    void setGroup(ThreadGroup* group);
    ThreadGroup* group();
    
    // ISPUChannelsThread
    inline uint8_t* ls(uint32_t i) override { return ptr(i); }
};

void set_mxcsr_for_spu();
