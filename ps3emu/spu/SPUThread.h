#pragma once

#include "ps3emu/libs/ConcurrentQueue.h"
#include "ps3emu/libs/sync/queue.h"
#include "ps3emu/constants.h"
#include "ps3emu/BitField.h"
#include "ps3emu/ReservationMap.h"
#include "ps3emu/enum.h"
#include "SPUChannels.h"
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
#include <experimental/optional>
#include <x86intrin.h>

static constexpr uint32_t LSLR = 0x3ffff;
static constexpr uint32_t LocalStorageSize = 256 * 1024;

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

struct alignas(16) R128 {
    __m128i _xmm;
public:
    R128() = default;
    inline R128(R128 const& r) {
        _xmm = r._xmm;
    }
    
    inline __m128i xmm() {
        return _xmm;
    }
    
    inline void set_xmm(__m128i xmm) {
        _xmm = xmm;
    }
    
    inline uint8_t b(int n) {
        uint8_t vec[16];
        memcpy(vec, &_xmm, 16);
        return vec[15 - n];
    }
    
    inline void set_b(int n, uint8_t val) {
        uint8_t vec[16];
        memcpy(vec, &_xmm, 16);
        vec[15 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline int16_t hw(int n) {
        int16_t vec[8];
        memcpy(vec, &_xmm, 16);
        return vec[7 - n];
    }
    
    inline void set_hw(int n, uint16_t val) {
        int16_t vec[8];
        memcpy(vec, &_xmm, 16);
        vec[7 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    template <int N>
    int32_t w() {
        return (int32_t)_mm_extract_epi32(_xmm, 3 - N);
    }
    
    inline int32_t w(int n) {
        int32_t vec[4];
        memcpy(vec, &_xmm, 16);
        return vec[3 - n];
    }
    
    inline void set_w(int n, uint32_t val) {
        int32_t vec[4];
        memcpy(vec, &_xmm, 16);
        vec[3 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    template <int N>
    int64_t dw() {
        return (int64_t)_mm_extract_epi64(_xmm, 1 - N);
    }
    
    inline int64_t dw(int n) {
        int64_t vec[2];
        memcpy(vec, &_xmm, 16);
        return vec[1 - n];
    }
    
    inline void set_dw(int n, int64_t val) {
        int64_t vec[2];
        memcpy(vec, &_xmm, 16);
        vec[1 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline float fs(int n) {
        float vec[4];
        memcpy(vec, &_xmm, 16);
        return vec[3 - n];
    }
    
    inline void set_fs(int n, float val) {
        float vec[4];
        memcpy(vec, &_xmm, 16);
        vec[3 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline double fd(int n) {
        double vec[2];
        memcpy(vec, &_xmm, 16);
        return vec[1 - n];
    }
    
    inline void set_fd(int n, double val) {
        double vec[2];
        memcpy(vec, &_xmm, 16);
        vec[1 - n] = val;
        memcpy(&_xmm, vec, 16);
    }
    
    inline int16_t hw_pref() {
        return hw(1);
    }
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

class SPUThread : boost::noncopyable, public ISPUChannelsThread {
    uint32_t _nip;
    R128 _rs[128];
    R128 _fpscr;
    uint8_t _ls[LocalStorageSize];
    uint32_t _srr0;
    uint32_t _spu;
    std::string _name;
    boost::thread _thread;
    SPUChannels _channels;
    std::function<void(SPUThread*, SPUThreadEvent)> _eventHandler;
    std::atomic<bool> _dbgPaused = false;
    std::atomic<bool> _singleStep = false;
    int32_t _exitCode;
    SPUThreadExitCause _cause;
    uint32_t _elfSource;
    std::experimental::optional<InterruptThreadInfo> _interruptHandler;
    uint64_t _id;
    boost::mutex _eventQueuesMutex;
    std::vector<EventQueueInfo> _eventQueuesToPPU;
    std::vector<EventQueueInfo> _eventQueuesToSPU;
    void loop();
    void handleInterrupt(uint32_t interruptValue);
    void handleReceiveEvent();
    bool _hasStarted;
    
    boost::mutex _groupExitMutex;
    boost::condition_variable _groupExitCv;
    bool _groupExitPending = false;
    std::function<std::vector<uint32_t>()> _getGroupThreads;
    ReservationGranule _granule;

public:
    SPUThread(std::string name,
              std::function<void(SPUThread*, SPUThreadEvent)> eventHandler);
#if TESTS
    SPUThread();
#endif
    
    template <typename V>
    inline R128& r(V i) {
        assert(getUValue(i) < 128);
        return _rs[getUValue(i)];
    }
    
    inline uint8_t* ptr(uint32_t lsa) {
        assert(lsa < sizeof(_ls));
        return &_ls[lsa];
    }
    
    inline void setNip(uint32_t nip) override {
        assert(nip < LSLR);
        _nip = nip;
    }
    
    inline uint32_t getNip() {
        return _nip;
    }
    
    inline uint32_t getSrr0() {
        return _srr0;
    }
    
    inline void setSrr0(uint32_t val) {
        assert(val < LSLR);
        _srr0 = val;
    }
    
    inline void setSpu(uint32_t num) {
        assert(num <= 255);
        _spu = num;
    }
    
    inline R128& fpscr() {
        return _fpscr;
    }
    
    inline uint32_t getSpu() {
        return _spu;
    }
    
    void singleStepBreakpoint();
    void dbgPause(bool val);
    bool dbgIsPaused();
    void run() override;
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
    void groupExit();
    void setGroup(std::function<std::vector<uint32_t>()> getThreads);
    
    // ISPUChannelsThread
    inline uint8_t* ls(uint32_t i) override { return ptr(i); }
};
