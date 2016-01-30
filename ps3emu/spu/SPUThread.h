#pragma once

#include "../../libs/ConcurrentQueue.h"
#include "../constants.h"
#include "../BitField.h"
#include <boost/thread.hpp>
#include <atomic>
#include <assert.h>
#include <algorithm>
#include <stdexcept>
#include <string.h>
#include <functional>

static constexpr uint32_t LSLR = 0x3ffff;
static constexpr uint32_t LocalStorageSize = 256 * 1024;

class StopSignalException : public virtual std::runtime_error {
public:
    StopSignalException() : std::runtime_error("stop signal") { }
};

enum class SPUThreadExitCause {
    Exit,
    GroupExit,
    GroupTerminate
};

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

class SPUThreadInterruptException : public virtual std::exception {};

enum class SPUThreadEvent {
    Breakpoint,
    SingleStepBreakpoint,
    Started,
    Finished,
    InvalidInstruction,
    Failure
};

class R128 {
    uint8_t _bs[16];
public:
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    template <int N>
    uint8_t& b() {
        static_assert(0 <= N && N < 16, "");
        return _bs[15 - N];
    }
    
    inline uint8_t& b(int n) {
        assert(0 <= n && n < 16);
        return _bs[15 - n];
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
    
    template <int N>
    int32_t& w() {
        static_assert(0 <= N && N < 4, "");
        return ((int32_t*)_bs)[3 - N];
    }
    
    inline int32_t& w(int n) {
        assert(0 <= n && n < 4);
        return ((int32_t*)_bs)[3 - n];
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

#define SPU_RdEventStat          0
#define SPU_WrEventMask          1
#define SPU_WrEventAck           2
#define SPU_RdSigNotify1         3
#define SPU_RdSigNotify2         4
#define SPU_WrDec                7
#define SPU_RdDec                8
#define SPU_RdEventStatMask     11
#define SPU_RdEventMask         11
#define SPU_RdMachStat          13
#define SPU_WrSRR0              14
#define SPU_RdSRR0              15
#define SPU_WrOutMbox           28 
#define SPU_RdInMbox            29 
#define SPU_WrOutIntrMbox       30 

#define MFC_WrMSSyncReq          9
#define MFC_RdTagMask           12
#define MFC_LSA                 16 
#define MFC_EAH                 17 
#define MFC_EAL                 18 
#define MFC_Size                19 
#define MFC_TagID               20 
#define MFC_Cmd                 21 
#define MFC_WrTagMask           22 
#define MFC_WrTagUpdate         23 
#define MFC_RdTagStat           24 
#define MFC_RdListStallStat     25 
#define MFC_WrListStallAck      26 
#define MFC_RdAtomicStat        27 

class Process;

struct SPUThreadExitInfo {
    SPUThreadExitCause cause;
    int32_t status;
};

class SPUThread {
    uint32_t _nip;
    R128 _rs[128];
    uint32_t _ch[28];
    uint8_t _ls[LocalStorageSize];
    uint32_t _srr0;
    uint32_t _spu;
    std::string _name;
    boost::thread _thread;
    Process* _proc;
    std::function<void(SPUThread*, SPUThreadEvent)> _eventHandler;
    std::atomic<bool> _dbgPaused;
    std::atomic<bool> _singleStep;
    int32_t _exitCode;
    SPUThreadExitCause _cause;
    uint32_t _elfSource;
    std::function<void()> _interruptHandler;
    ConcurrentFifoQueue<uint32_t> _toSpuMailbox;
    ConcurrentFifoQueue<uint32_t> _fromSpuMailbox;
    ConcurrentFifoQueue<uint32_t> _fromSpuInterruptMailbox;
    std::atomic<uint32_t> _status;
    void loop();
    
public:
    SPUThread(Process* proc,
              std::string name,
              std::function<void(SPUThread*, SPUThreadEvent)> eventHandler);
    
    template <typename V>
    inline R128& r(V i) {
        return _rs[getUValue(i)];
    }
    
    template <typename V>
    inline uint32_t& ch(V i) {
        return _ch[getUValue(i)];
    }
    
    inline uint8_t* ptr(uint32_t lsa) {
        assert(lsa < sizeof(_ls));
        return &_ls[lsa];
    }
    
    inline void setNip(uint32_t nip) {
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
    
    inline Process* proc() {
        return _proc;
    }
    
    void singleStepBreakpoint();
    void dbgPause(bool val);
    void run();
    SPUThreadExitInfo join();
    void setElfSource(uint32_t src);
    uint32_t getElfSource();
    void command(uint32_t word);
    void setInterruptHandler(std::function<void()> interruptHandler);
    ConcurrentFifoQueue<uint32_t>& getFromSpuMailbox();
    ConcurrentFifoQueue<uint32_t>& getFromSpuInterruptMailbox();
    ConcurrentFifoQueue<uint32_t>& getToSpuMailbox();
    std::atomic<uint32_t>& getStatus();
};