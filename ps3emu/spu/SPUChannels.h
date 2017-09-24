#pragma once

#include "ps3emu/libs/ConcurrentBoundedQueue.h"
#include "ps3emu/enum.h"
#include <stdexcept>
#include <array>
#include <atomic>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include "stdint.h"

ENUMF(MfcEvent,
    (MFC_TAG_STATUS_UPDATE_EVENT, 0x00000001),
    (MFC_LIST_STALL_NOTIFY_EVENT, 0x00000002),
    (MFC_COMMAND_QUEUE_AVAILABLE_EVENT, 0x00000008),
    (MFC_IN_MBOX_AVAILABLE_EVENT, 0x00000010),
    (MFC_DECREMENTER_EVENT, 0x00000020),
    (MFC_OUT_INTR_MBOX_AVAILABLE_EVENT, 0x00000040),
    (MFC_OUT_MBOX_AVAILABLE_EVENT, 0x00000080),
    (MFC_SIGNAL_NOTIFY_2_EVENT, 0x00000100),
    (MFC_SIGNAL_NOTIFY_1_EVENT, 0x00000200),
    (MFC_LLR_LOST_EVENT, 0x00000400),
    (MFC_PRIV_ATTN_EVENT, 0x00000800),
    (MFC_MULTI_SRC_SYNC_EVENT, 0x00001000))

ENUM(MfcTag,
    (MFC_TAG_UPDATE_IMMEDIATE, 0x0),
    (MFC_TAG_UPDATE_ANY, 0x1),
    (MFC_TAG_UPDATE_ALL, 0x2))

#define X(k, v) k = v,
#define SpuChannelsX \
    X(SPU_RdEventStat    , 0) \
    X(SPU_WrEventMask    , 1) \
    X(SPU_WrEventAck     , 2) \
    X(SPU_RdSigNotify1   , 3) \
    X(SPU_RdSigNotify2   , 4) \
    X(SPU_WrDec          , 7) \
    X(SPU_RdDec          , 8) \
    X(MFC_WrMSSyncReq    , 9) \
    X(SPU_RdEventMask    ,11) \
    X(MFC_RdTagMask      ,12) \
    X(SPU_RdMachStat     ,13) \
    X(SPU_WrSRR0         ,14) \
    X(SPU_RdSRR0         ,15) \
    X(MFC_LSA            ,16) \
    X(MFC_EAH            ,17) \
    X(MFC_EAL            ,18) \
    X(MFC_Size           ,19) \
    X(MFC_TagID          ,20) \
    X(MFC_Cmd            ,21) \
    X(MFC_WrTagMask      ,22) \
    X(MFC_WrTagUpdate    ,23) \
    X(MFC_RdTagStat      ,24) \
    X(MFC_RdListStallStat,25) \
    X(MFC_WrListStallAck ,26) \
    X(MFC_RdAtomicStat   ,27) \
    X(SPU_WrOutMbox      ,28) \
    X(SPU_RdInMbox       ,29) \
    X(SPU_WrOutIntrMbox  ,30) \
    X(SPU_PerfBookmark   ,69)

enum SpuChannels { SpuChannelsX };
#undef X

#define X(k, v) k = v,
#define TagClassIdX \
    X(_MFC_LSA, 0x3004U) \
    X(_MFC_EAH, 0x3008U) \
    X(_MFC_EAL, 0x300CU) \
    X(MFC_Size_Tag, 0x3010U) \
    X(MFC_Class_CMD, 0x3014U) \
    X(MFC_QStatus, 0x3104U) \
    X(Prxy_QueryType, 0x3204U) \
    X(Prxy_QueryMask, 0x321CU) \
    X(Prxy_TagStatus, 0x322CU) \
    X(SPU_Out_Intr_Mbox, 0x4000U) \
    X(SPU_Out_MBox, 0x4004U) \
    X(SPU_In_MBox, 0x400CU) \
    X(SPU_MBox_Status, 0x4014U) \
    X(SPU_RunCntl, 0x401CU) \
    X(SPU_Status, 0x4024U) \
    X(SPU_NPC, 0x4034U) \
    X(SPU_Sig_Notify_1, 0x1400CU) \
    X(SPU_Sig_Notify_2, 0x1C00CU)
    
enum TagClassId {
    TagClassIdX
};
#undef X

#define X(k, v) k = v,
#define MfcCommandX \
    X(MFC_PUT_CMD     ,0x0020)                 \
    X(MFC_PUTS_CMD    ,0x0028)  /*  PU Only */ \
    X(MFC_PUTR_CMD    ,0x0030)                 \
    X(MFC_PUTF_CMD    ,0x0022)                 \
    X(MFC_PUTB_CMD    ,0x0021)                 \
    X(MFC_PUTFS_CMD   ,0x002A)  /*  PU Only */ \
    X(MFC_PUTBS_CMD   ,0x0029)  /*  PU Only */ \
    X(MFC_PUTRF_CMD   ,0x0032)                 \
    X(MFC_PUTRB_CMD   ,0x0031)                 \
    X(MFC_PUTL_CMD    ,0x0024)  /* SPU Only */ \
    X(MFC_PUTRL_CMD   ,0x0034)  /* SPU Only */ \
    X(MFC_PUTLF_CMD   ,0x0026)  /* SPU Only */ \
    X(MFC_PUTLB_CMD   ,0x0025)  /* SPU Only */ \
    X(MFC_PUTRLF_CMD  ,0x0036)  /* SPU Only */ \
    X(MFC_PUTRLB_CMD  ,0x0035)  /* SPU Only */ \
    X(MFC_GET_CMD     ,0x0040)                 \
    X(MFC_GETS_CMD    ,0x0048)  /*  PU Only */ \
    X(MFC_GETF_CMD    ,0x0042)                 \
    X(MFC_GETB_CMD    ,0x0041)                 \
    X(MFC_GETFS_CMD   ,0x004A)  /*  PU Only */ \
    X(MFC_GETBS_CMD   ,0x0049)  /*  PU Only */ \
    X(MFC_GETL_CMD    ,0x0044)  /* SPU Only */ \
    X(MFC_GETLF_CMD   ,0x0046)  /* SPU Only */ \
    X(MFC_GETLB_CMD   ,0x0045)  /* SPU Only */ \
    X(MFC_GETLLAR_CMD ,0x00d0)                 \
    X(MFC_PUTLLC_CMD  ,0x00b4)                 \
    X(MFC_PUTLLUC_CMD ,0x00b0)                 \
    X(MFC_PUTQLLUC_CMD,0x00b8)

enum MfcCommands { MfcCommandX };
#undef X

enum SPU_Status_Flags {
    SPU_Status_E = 1u << (31u - 21u),
    SPU_Status_L = 1u << (31u - 22u),
    SPU_Status_IS = 1u << (31u - 24u),
    SPU_Status_C= 1u << (31u - 25u),
    SPU_Status_I = 1u << (31u - 26u),
    SPU_Status_S = 1u << (31u - 27u),
    SPU_Status_W = 1u << (31u - 28u),
    SPU_Status_H = 1u << (31u - 29u),
    SPU_Status_P = 1u << (31u - 30u),
    SPU_Status_R = 1u << (31u - 31u)
};

#define SPU_Status_SetStopCode(status, code) (status = ((status & 0xffff) | (code << 16u)))
#define SPU_Status_GetStopCode(status) ((status >> 16) & 0xffff)

enum INT_Mask_class2_Flags {
    INT_Mask_class2_B = 1ull << (63u - 59u),
    INT_Mask_class2_T = 1ull << (63u - 60u),
    INT_Mask_class2_H = 1ull << (63u - 61u),
    INT_Mask_class2_S = 1ull << (63u - 62u),
    INT_Mask_class2_M = 1ull << (63u - 63u)
};

class SPUThreadInterruptException : public virtual std::exception {
    uint32_t _imboxValue;

public:
    inline SPUThreadInterruptException(uint32_t imboxValue)
        : _imboxValue(imboxValue) {}
    inline uint32_t imboxValue() {
        return _imboxValue;
    }
};

class ISPUChannelsThread {
public:
    virtual void run(bool suspended) = 0;
    virtual void setNip(uint32_t) = 0;
    virtual uint8_t* ls(uint32_t i) = 0;
    virtual ~ISPUChannelsThread() = default;
};

class SpuEvent {
    boost::mutex _m;
    boost::condition_variable _cv;
    unsigned _pending = 0;
    unsigned _mask = 0;
    unsigned _count = 0;
    void updateCount();
        
public:
    void setMask(unsigned mask);
    unsigned mask();
    void acknowledge(unsigned mask);
    unsigned wait();
    unsigned wait_clear();
    unsigned count();
    unsigned pending();
    void set_or(unsigned flags);
    void set(unsigned flags);
};

class SpuSignal {
    boost::mutex _m;
    boost::condition_variable _cv;
    unsigned _value = 0;
    unsigned _count = 0;
    void updateCount();
        
public:
    unsigned wait_clear();
    unsigned count();
    unsigned value();
    void set_or(unsigned value);
    void set(unsigned value);
};

class MainMemory;
class SPUThread;
class SPUChannels {
    MainMemory* _mm;
    ISPUChannelsThread* _thread;
    SPUThread* _sthread;
    ConcurrentBoundedQueue<uint32_t> _outboundMailbox;
    ConcurrentBoundedQueue<uint32_t> _outboundInterruptMailbox;
    ConcurrentBoundedQueue<uint32_t> _inboundMailbox;
    std::array<std::atomic<uint32_t>, SPU_PerfBookmark + 1> _channels;
    std::atomic<uint32_t> _spuStatus;
    std::atomic<uint32_t> _interrupt2;
    SpuEvent _event;
    SpuSignal _snr1, _snr2;
    void command(uint32_t word);
    
public:
    SPUChannels(MainMemory* mm, ISPUChannelsThread* thread, SPUThread* sthread = nullptr);
    void write(unsigned channel, uint32_t data);
    uint32_t read(unsigned channel);
    unsigned readCount(unsigned channel);
    void mmio_write(unsigned offset, uint64_t data);
    uint32_t mmio_read(unsigned offset);
    unsigned mmio_readCount(unsigned offset);
    void setEvent(unsigned flags);
    void silently_write_interrupt_mbox(uint32_t value);
    inline std::atomic<uint32_t>& spuStatus() { return _spuStatus; }
    inline std::atomic<uint32_t>& interrupt() { return _interrupt2; }
};
