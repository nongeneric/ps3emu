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

class alignas(16) R128 {
    uint8_t _bs[16];
public:
    R128() = default;
    inline R128(R128 const& r) {
        memcpy(_bs, r._bs, sizeof(_bs));
    }
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    inline __m128i xmm() {
        return _mm_lddqu_si128((__m128i*)_bs);
    }
    
    inline void set_xmm(__m128i xmm) {
        _mm_store_si128((__m128i*)_bs, xmm);
    }

    template <int N>
    uint8_t& b() {
        static_assert(0 <= N && N < 16, "");
        return _bs[15 - N];
    }
    
    inline uint8_t& b(int n) {
        assert(0 <= n && n < 16);
        return _bs[15 - n];
    }
    
    inline uint8_t& b_pref() {
        return b<3>();
    }
    
    template <int N>
    int16_t& hw() {
        static_assert(0 <= N && N < 8, "");
        return ((int16_t*)_bs)[7 - N];
    }
    
    inline int16_t& hw(int n) {
        assert(0 <= n && n < 8);
        return ((int16_t*)_bs)[7 - n];
    }
    
    inline int16_t& hw_pref() {
        return hw<1>();
    }
    
    template <int N>
    int32_t& w() {
        static_assert(0 <= N && N < 4, "");
        return ((int32_t*)_bs)[3 - N];
    }
    
    inline int32_t& w(int n) {
        assert(0 <= n && n < 4);
        return ((int32_t*)_bs)[3 - n];
    }
    
    inline int32_t& w_pref() {
        return w<0>();
    }
    
    template <int N>
    int64_t& dw() {
        static_assert(0 <= N && N < 2, "");
        return ((int64_t*)_bs)[1 - N];
    }
    
    inline int64_t& dw(int n) {
        assert(0 <= n && n < 2);
        return ((int64_t*)_bs)[1 - n];
    }
    
    template <int N>
    float& fs() {
        static_assert(0 <= N && N < 4, "");
        return ((float*)_bs)[3 - N];
    }
    
    inline float& fs(int n) {
        assert(0 <= n && n < 4);
        return ((float*)_bs)[3 - n];
    }
    
    template <int N>
    double& fd() {
        static_assert(0 <= N && N < 2, "");
        return ((double*)_bs)[1 - N];
    }
    
    inline double& fd(int n) {
        assert(0 <= n && n < 2);
        return ((double*)_bs)[1 - n];
    }
    
    inline void load(const uint8_t* ptr) {
        std::reverse_copy(ptr, ptr + 16, _bs);
    }
    
    inline void store(uint8_t* ptr) {
        std::reverse_copy(_bs, _bs + 16, ptr);
    }
#pragma GCC diagnostic pop
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
