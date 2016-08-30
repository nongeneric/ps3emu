#include "SPUChannels.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include "ps3emu/BitField.h"
#include "ps3emu/MainMemory.h"
#include <stdexcept>
#include <algorithm>

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

#define X(k, v) case k: return #k;
const char* mfcCommandToString(int command) {
    switch (command) { MfcCommandX }
    return "unknown";
}
#undef X

#define X(k, v) case k: return #k;
const char* classIdToString(int id) {
    switch (id) { SpuChannelsX }
    return "unknown";
}
#undef X

#define X(k, v) case k: return #k;
const char* tagToString(TagClassId tag) {
    switch (tag) { TagClassIdX }
    return "unknown";
}
#undef X

SPUChannels::SPUChannels(MainMemory* mm, ISPUChannelsThread* thread)
    : _mm(mm),
      _thread(thread),
      _outboundMailbox(1),
      _outboundInterruptMailbox(1),
      _inboundMailbox(4) {
    std::fill(begin(_channels), end(_channels), 0);
    _channels[MFC_WrTagMask] = -1;
}

void SPUChannels::command(uint32_t word) {
    union {
        uint32_t val;
        BitField<0, 8> tid;
        BitField<8, 16> rid;
        BitField<16, 32> opcode;
    } cmd = { word };
    assert(cmd.tid.u() == 0);
    assert(cmd.rid.u() == 0);
    auto eal = _channels[MFC_EAL].load();
    auto lsa = &_thread->ls()[_channels[MFC_LSA]];
    auto size = _channels[MFC_Size].load();
    auto opcode = cmd.opcode.u();
    switch (opcode) {
        case MFC_GETLLAR_CMD: {
            _mm->readReserve(eal, lsa, size);
            // reservation always succeeds
            _channels[MFC_RdAtomicStat] |= 0b100; // G
            break;
        }
        case MFC_GET_CMD:
        case MFC_GETS_CMD:
        case MFC_GETF_CMD:
        case MFC_GETB_CMD:
        case MFC_GETFS_CMD:
        case MFC_GETBS_CMD: {
            // readMemory always synchronizes
            _mm->readMemory(eal, lsa, size);
            break;
        }
        case MFC_PUTLLC_CMD: // TODO: handle sizes correctly when calling writeCond
        case MFC_PUTLLUC_CMD:
        case MFC_PUTQLLUC_CMD: {
            assert(opcode != MFC_PUTQLLUC_CMD);
            auto stored = _mm->writeCond(eal, lsa, size);
            if (opcode == MFC_PUTLLUC_CMD) {
                if (!stored) {
                    _mm->writeMemory(eal, lsa, size);
                } else {
                    _channels[MFC_RdAtomicStat] |= 0b010; // U
                }
            } else if (opcode == MFC_PUTLLC_CMD) {
                _channels[MFC_RdAtomicStat] |= !stored; // S
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
            // writeMemory always synchronizes
            _mm->writeMemory(eal, lsa, size);
            break;
        }
        default: throw std::runtime_error("not implemented");
    }
}

unsigned SPUChannels::readCount(unsigned ch) {
    auto count = ([ch, this] {
        switch (ch) {
            //case SPU_RdEventStat: return 0;
            case SPU_WrEventMask: return 1u;
            case SPU_WrEventAck: return 1u;
            //case SPU_RdSigNotify1: return 0;
            //case SPU_RdSigNotify2: return 0;
            case SPU_WrDec: return 1u;
            case SPU_RdDec: return 1u;
            case SPU_RdEventMask: return 1u;
            case SPU_RdMachStat: return 1u;
            case SPU_WrSRR0: return 1u;
            case SPU_RdSRR0: return 1u;
            case SPU_WrOutMbox: return 1u - _outboundMailbox.size();
            case SPU_RdInMbox: return _inboundMailbox.size();
            case SPU_WrOutIntrMbox: return 1u - _outboundInterruptMailbox.size();
            case MFC_WrMSSyncReq: return 1u;
            case MFC_RdTagMask: return 1u;
            case MFC_LSA: return 1u;
            case MFC_EAH: return 1u;
            case MFC_EAL: return 1u;
            case MFC_Size: return 1u;
            case MFC_TagID: return 1u;
            // mfc commands execute immediately, the queue is always empty
            case MFC_Cmd: return 16u;
            case MFC_WrTagMask: return 1u;
            case MFC_WrTagUpdate: return 1u; // immediate mfc
            case MFC_RdTagStat: return 1u; // immediate mfc
            //case MFC_RdListStallStat: return 0;
            //case MFC_WrListStallAck: return 0;
            //case MFC_RdAtomicStat: return 0;
            default: assert(false); return 0u;
        }
    })();
    INFO(spu) << ssnprintf("spu reads count %d from channel %s",
                           count,
                           classIdToString(ch));
    return count;
}

void SPUChannels::write(unsigned ch, uint32_t data) {
    if (ch == SPU_WrOutIntrMbox) {
        _outboundInterruptMailbox.enqueue(data);
        _interrupt2 |= INT_Mask_class2_M;
        INFO(spu) << ssnprintf("spu writes %x to interrupt mailbox", data);
        throw SPUThreadInterruptException();
    } else if (ch == SPU_WrOutMbox) {
        _outboundMailbox.enqueue(data);
    } else {
        if (ch == MFC_Cmd) {
            command(data);
        } else {
            _channels.at(ch) = data;
        }
    }
    INFO(spu) << ssnprintf("spu writes %x%s to channel %d (%s)",
                           data,
                           ch == MFC_Cmd ? ssnprintf(" (%s)", mfcCommandToString(data))
                                         : "",
                           ch,
                           classIdToString(ch));
}

uint32_t SPUChannels::read(unsigned ch) {
    auto data = ([&ch, this] {
        if (ch == SPU_RdInMbox) {
            return _inboundMailbox.dequeue();
        } else if (ch == MFC_RdAtomicStat) {
            auto res = _channels[ch].load();
            _channels[ch] = 0;
            return res;
        } else {
            if (ch == MFC_RdTagStat) {
                // as every MFC request completes immediately
                // it is possible to just always return the mask set last
                ch = MFC_WrTagMask;
            }
            return _channels[ch].load();
        }
    })();
    INFO(spu) << ssnprintf(
        "spu reads %x from channel %d (%s)", data, ch, classIdToString(ch));
    return data;
}

void SPUChannels::mmio_write(unsigned offset, uint64_t data) {
    INFO(spu) << ssnprintf("ppu writes %x via mmio to spu %d tag %s",
                           data,
                           -1,
                           tagToString((TagClassId)offset));

    if (offset == TagClassId::SPU_RunCntl && data == 1) {
        _thread->run();
        _spuStatus |= SPU_Status_R;
        return;
    }
    if (offset == TagClassId::SPU_In_MBox) {
        _inboundMailbox.enqueue(data);
        return;
    }
    if (offset == TagClassId::_MFC_LSA) {
        _channels[MFC_LSA] = data;
        return;
    }
    if (offset == TagClassId::_MFC_EAH) {
        _channels[MFC_EAH] = data;
        return;
    }
    if (offset == TagClassId::_MFC_EAL) {
        _channels[MFC_EAL] = data;
        return;
    }
    if (offset == TagClassId::MFC_Size_Tag) {
        _channels[MFC_Size] = data >> 16;
        _channels[MFC_TagID] = data & 0xff;
        return;
    }
    if (offset == TagClassId::MFC_Class_CMD) {
        command(data);
        return;
    }
    if (offset == TagClassId::Prxy_QueryMask) {
        _channels[MFC_WrTagMask] = data;
        return;
    }
    if (offset == TagClassId::Prxy_QueryType) {
        // do nothing, every request is completed immediately and
        // as such there is no difference between any (01) and all (10) tag groups
        // also disabling completion (00) notifications makes no sense
        return;
    }
    if (offset == TagClassId::SPU_NPC) {
        _thread->setNip(data);
        return;
    }
    throw std::runtime_error("unknown mmio offset");
}

uint32_t SPUChannels::mmio_read(unsigned offset) {
    auto res = [&] {
        switch (offset) {
            case TagClassId::SPU_Status: return _spuStatus.load();
            case TagClassId::SPU_Out_MBox: return _outboundMailbox.dequeue();
            case TagClassId::SPU_Out_Intr_Mbox: return _outboundInterruptMailbox.dequeue();
            case TagClassId::MFC_Class_CMD: return _channels[MFC_Cmd].load();
            // all mfc requests complete immediately
            case TagClassId::MFC_QStatus: return 0x8000ffffu;
            case TagClassId::SPU_MBox_Status:
                return (mmio_readCount(SPU_Out_Intr_Mbox) << 16) |
                       (mmio_readCount(SPU_In_MBox) << 8) |
                       mmio_readCount(SPU_Out_MBox);
            case TagClassId::Prxy_QueryType: return 0x01u;
            case TagClassId::Prxy_TagStatus: return -1u;
            default: throw std::runtime_error("unknown mmio offset");
        }
    }();
    INFO(spu) << ssnprintf("ppu reads %x via mmio from spu %d tag %s",
                           res,
                           -1,
                           tagToString((TagClassId)offset));
    return res;
}

unsigned SPUChannels::mmio_readCount(unsigned offset) {
    if (offset == TagClassId::SPU_Out_MBox) {
        return _outboundMailbox.size();
    }
    if (offset == TagClassId::SPU_Out_Intr_Mbox) {
        return _outboundInterruptMailbox.size();
    }
    if (offset == TagClassId::SPU_In_MBox) {
        return 4u - _inboundMailbox.size();
    }
    throw std::runtime_error("unknown mmio offset");
}
