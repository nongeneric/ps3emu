#include "SPUChannels.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include "ps3emu/BitField.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/state.h"
#include "ps3emu/Process.h"
#include <stdexcept>
#include <algorithm>

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

void SpuEvent::set_or(unsigned flags) {
    boost::unique_lock<boost::mutex> lock(_m);
    _pending |= flags;
    updateCount();
    _cv.notify_all();
}

unsigned SpuEvent::count() {
    boost::unique_lock<boost::mutex> lock(_m);
    return _count;
}

unsigned SpuEvent::wait() {
    boost::unique_lock<boost::mutex> lock(_m);
    _cv.wait(lock, [&] { return _count; });
    return _pending & _mask;
}

void SpuEvent::acknowledge(unsigned mask) {
    boost::unique_lock<boost::mutex> lock(_m);
    _pending &= ~mask;
    _count = 0;
}

void SpuEvent::setMask(unsigned mask) {
    boost::unique_lock<boost::mutex> lock(_m);
    _mask = mask;
    updateCount();
    _cv.notify_all();
}

unsigned SpuEvent::mask() {
    boost::unique_lock<boost::mutex> lock(_m);
    return _mask;
}

void SpuEvent::updateCount() {
    _count = (_pending & _mask) != 0;
}

unsigned SpuSignal::value() {
    boost::unique_lock<boost::mutex> lock(_m);
    return _value;
}

unsigned SpuSignal::wait_clear() {
    boost::unique_lock<boost::mutex> lock(_m);
    _cv.wait(lock, [&] { return _count; });
    auto res = _value;
    _count = 0;
    _value = 0;
    return res;
}

unsigned SpuSignal::count() {
    boost::unique_lock<boost::mutex> lock(_m);
    return _count;
}

void SpuSignal::set_or(unsigned value) {
    boost::unique_lock<boost::mutex> lock(_m);
    _value |= value;
    _count = 1;
    _cv.notify_all();
}

void SpuSignal::set(unsigned value) {
    boost::unique_lock<boost::mutex> lock(_m);
    _value = value;
    _count = 1;
    _cv.notify_all();
}

SPUChannels::SPUChannels(MainMemory* mm, ISPUChannelsThread* thread)
    : _mm(mm),
      _thread(thread),
      _outboundMailbox(1),
      _outboundInterruptMailbox(1),
      _inboundMailbox(4) {
    std::fill(begin(_channels), end(_channels), 0);
    _channels[MFC_WrTagMask] = -1;
}

#define STALL_AND_NOTIFY(x) (x >> 63ul)
#define LTS(x) ((x >> 32ul) & 0x7fffful)
#define LEAL(x) (x & 0xfffffffful)

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
    assert(_channels[MFC_EAH] == 0);
    auto lsaVa = _channels[MFC_LSA].load();
    auto lsa = _thread->ls(lsaVa);
    auto size = _channels[MFC_Size].load();
    auto opcode = cmd.opcode.u();
    auto name = mfcCommandToString(opcode);
    auto tag = _channels[MFC_TagID].load();
    auto log = [&] {
        INFO(spu) << ssnprintf("%s(%x, %x, %x) %x", name, size, lsaVa, eal, tag);
    };
    auto logAtomic = [&](bool stored) {
        if (stored) {
            INFO(spu) << ssnprintf(
                "%s(%x, %x, %x) %s", name, size, lsaVa, eal, stored ? "OK" : "FAIL");
        }
    };
    switch (opcode) {
        case MFC_GETLLAR_CMD: {
            auto hostThreadId = boost::this_thread::get_id();
            _mm->loadReserve(eal, lsa, size, [=](auto id) {
                if (hostThreadId != id) {
                    _event.set_or((unsigned)MfcEvent::MFC_LLR_LOST_EVENT);
                }
            });
            // reservation always succeeds
            _channels[MFC_RdAtomicStat] |= 0b100; // G
            log();
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
            log();
            break;
        }
        case MFC_PUTLLC_CMD:
        case MFC_PUTQLLUC_CMD: {
            assert(opcode != MFC_PUTQLLUC_CMD);
            assert(size == 0x80);
            auto stored = _mm->writeCond(eal, lsa, size);
            _channels[MFC_RdAtomicStat] |= !stored; // S
            logAtomic(stored);
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
        case MFC_PUTLLUC_CMD:
        case MFC_PUTRB_CMD: {
            if (opcode == MFC_PUTLLUC_CMD) {
                assert(size == 0x80); // cache line
                _channels[MFC_RdAtomicStat] |= 0b010; // U
            }
            // writeMemory always synchronizes
            _mm->writeMemory(eal, lsa, size);
            log();
            break;
        }
        case MFC_GETL_CMD:
        case MFC_GETLF_CMD:
        case MFC_GETLB_CMD: {
            assert(size % sizeof(big_uint64_t) == 0);
            auto list = (const big_uint64_t*)_thread->ls(eal);
            for (auto i = 0u; i < size / sizeof(big_uint64_t); ++i) {
                assert(!STALL_AND_NOTIFY(list[i]));
                auto cursize = LTS(list[i]);
                _mm->readMemory(LEAL(list[i]), lsa, cursize);
                lsa += std::max(8ul, cursize);
            }
            break;
        }
        case MFC_PUTL_CMD:
        case MFC_PUTRL_CMD:
        case MFC_PUTLF_CMD:
        case MFC_PUTLB_CMD:
        case MFC_PUTRLF_CMD:
        case MFC_PUTRLB_CMD: {
            assert(size % sizeof(big_uint64_t) == 0);
            auto list = (const big_uint64_t*)_thread->ls(eal);
            for (auto i = 0u; i < size / sizeof(big_uint64_t); ++i) {
                assert(!STALL_AND_NOTIFY(list[i]));
                auto cursize = LTS(list[i]);
                _mm->writeMemory(LEAL(list[i]), lsa, cursize);
                lsa += std::max(8ul, cursize);
            }
            break;
        }   
        default: assert(false); throw std::runtime_error("not implemented");
    }
}

unsigned SPUChannels::readCount(unsigned ch) {
    auto count = ([ch, this] {
        switch (ch) {
            case SPU_RdEventStat: return _event.count();
            case SPU_WrEventMask: return 1u;
            case SPU_WrEventAck: return 1u;
            case SPU_RdSigNotify1: return _snr1.count();
            case SPU_RdSigNotify2: return _snr2.count();
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
    INFO(spu) << ssnprintf("read count %d from channel %s", count, classIdToString(ch));
    return count;
}

void SPUChannels::write(unsigned ch, uint32_t data) {
    assert(ch <= SPU_WrOutIntrMbox);
    if (ch == SPU_WrEventMask) {
        _event.setMask(data);
    } else if (ch == SPU_WrEventAck) {
        _event.acknowledge(data);
    } else if (ch == SPU_WrOutIntrMbox) {
        _interrupt2 |= INT_Mask_class2_M;
        INFO(spu) << ssnprintf("write %x to interrupt mailbox", data);
        throw SPUThreadInterruptException(data);
    } else if (ch == SPU_WrOutMbox) {
        _outboundMailbox.enqueue(data);
    } else if (ch == MFC_WrTagUpdate) {
        _channels[MFC_WrTagUpdate] = data;
        _channels[MFC_RdTagStat] = _channels[MFC_RdTagMask].load();
    } else if (ch == MFC_WrTagMask) {
        _channels[MFC_RdTagMask] = data;
    } else {
        if (ch == MFC_Cmd) {
            command(data);
        } else {
            _channels.at(ch) = data;
        }
    }
    if (ch == MFC_Cmd || ch == MFC_EAH || ch == MFC_EAL || ch == MFC_LSA ||
        ch == MFC_Size || ch == MFC_TagID || ch == MFC_WrTagMask ||
        ch == MFC_WrTagUpdate || ch == SPU_WrEventMask)
        return;
    
    std::string datastr;
    if (ch == SPU_WrEventMask || ch == SPU_WrEventAck) {
        datastr = to_string((MfcEvent)data);
    } else if (ch == MFC_WrTagUpdate) {
        datastr = to_string((MfcTag)data);
    } else {
        datastr = ssnprintf("%x", data);
    }
    INFO(spu) << ssnprintf("write %s to channel %d (%s)", datastr, ch, classIdToString(ch));
}

uint32_t SPUChannels::read(unsigned ch) {
    assert(ch <= SPU_WrOutIntrMbox);
    auto data = ([&] {
        if (ch == SPU_RdSigNotify1) {
            return _snr1.wait_clear();
        } else if (ch == SPU_RdSigNotify2) {
            return _snr2.wait_clear();
        } else if (ch == SPU_RdEventMask) {  
            return _event.mask();
        } else if (ch == SPU_RdEventStat) {
            return _event.wait();
        } else if (ch == SPU_RdInMbox) {
            return _inboundMailbox.dequeue();
        } else if (ch == MFC_RdAtomicStat) {
            auto res = _channels[ch].load();
            _channels[ch] = 0;
            return res;
        } else if (ch == SPU_RdDec) {
            return ~(uint32_t)g_state.proc->getTimeBase();
        } else {
            return _channels[ch].load();
        }
    })();
    if (ch == SPU_RdEventStat) {
        auto maskstr = to_string((MfcEvent)_event.mask());
        auto datastr = to_string((MfcEvent)data);
        INFO(spu) << ssnprintf("SPU_RdEventStat(mask: %s, ret: %s)", maskstr, datastr);
    } else if (ch == MFC_RdTagStat) {
        INFO(spu) << ssnprintf("MFC_RdTagStat(%x, %s) %x",
                               _channels[MFC_RdTagMask].load(),
                               to_string((MfcTag)_channels[MFC_WrTagUpdate].load()),
                               data);
    } else if (ch != MFC_RdAtomicStat) {
        INFO(spu) << ssnprintf("spu reads %x from channel %d (%s)",
                               data,
                               ch,
                               classIdToString(ch));
    } 
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
        _channels[MFC_RdTagMask] = data;
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
    if (offset == TagClassId::SPU_Sig_Notify_1) {
        _snr1.set(data); // TODO: check SPU_Cfg
        return;
    }
    if (offset == TagClassId::SPU_Sig_Notify_2) {
        _snr2.set(data); // TODO: check SPU_Cfg
        return;
    }
    throw std::runtime_error("unknown mmio offset");
}

uint32_t SPUChannels::mmio_read(unsigned offset) {
    INFO(spu) << "reading mmio";
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
            case TagClassId::SPU_Sig_Notify_1: return _snr1.value();
            case TagClassId::SPU_Sig_Notify_2: return _snr2.value();
            default: throw std::runtime_error("unknown mmio offset");
        }
    }();
    INFO(spu) << ssnprintf("read %x via mmio from spu %d tag %s",
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

void SPUChannels::setEvent(unsigned flags) {
    _event.set_or(flags);
}

void SPUChannels::silently_write_interrupt_mbox(uint32_t value) {
    _outboundInterruptMailbox.enqueue(value);
}
