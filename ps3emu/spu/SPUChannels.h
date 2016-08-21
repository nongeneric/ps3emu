#pragma once

#include "ps3emu/libs/ConcurrentBoundedQueue.h"
#include <stdexcept>
#include <array>
#include <atomic>
#include "stdint.h"

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
    X(SPU_WrOutIntrMbox  ,30)

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

#define SPU_Status_SetStopCode(status, code) ((status & 0xffff) | (code << 16u))
#define SPU_Status_GetStopCode(status) ((status >> 16) & 0xffff)

enum INT_Mask_class2_Flags {
    INT_Mask_class2_B = 1u << (63u - 59u),
    INT_Mask_class2_T = 1u << (63u - 60u),
    INT_Mask_class2_H = 1u << (63u - 61u),
    INT_Mask_class2_S = 1u << (63u - 62u),
    INT_Mask_class2_M = 1u << (63u - 63u)
};

class SPUThreadInterruptException : public virtual std::exception {};

class ISPUChannelsThread {
public:
    virtual void run() = 0;
    virtual void setNip(uint32_t) = 0;
    virtual uint8_t* ls() = 0;
    virtual ~ISPUChannelsThread() = default;
};

class MainMemory;
class SPUChannels {
    MainMemory* _mm;
    ISPUChannelsThread* _thread;
    ConcurrentBoundedQueue<uint32_t> _outboundMailbox;
    ConcurrentBoundedQueue<uint32_t> _outboundInterruptMailbox;
    ConcurrentBoundedQueue<uint32_t> _inboundMailbox;
    std::array<std::atomic<uint32_t>, 28> _channels;
    std::atomic<uint32_t> _spuStatus;
    std::atomic<uint32_t> _interrupt2;
    void command(uint32_t word);
    
public:
    SPUChannels(MainMemory* mm, ISPUChannelsThread* thread);
    void write(unsigned channel, uint32_t data);
    uint32_t read(unsigned channel);
    unsigned readCount(unsigned channel);
    void mmio_write(unsigned offset, uint64_t data);
    uint32_t mmio_read(unsigned offset);
    unsigned mmio_readCount(unsigned offset);
    inline std::atomic<uint32_t>& spuStatus() { return _spuStatus; }
    inline std::atomic<uint32_t>& interrupt() { return _interrupt2; }
};
