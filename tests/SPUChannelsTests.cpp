#include "ps3emu/MainMemory.h"
#include "ps3emu/log.h"
#include <catch2/catch.hpp>
#include "ps3emu/spu/SPUChannels.h"
#include "ps3emu/spu/SPUThread.h"
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

class TestSPUChannelsThread : public ISPUChannelsThread {
public:
    bool _run = false;
    uint32_t _nip = 0;
    std::array<uint8_t, LocalStorageSize> _ls;
    void run(bool suspended) override { _run = true; }
    void setNip(uint32_t nip) override { _nip = nip; }
    uint8_t* ls(uint32_t i) override { return &_ls[i]; }
};

TEST_CASE("spuchannels_mailbox_basic") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    // write to mbox from spu
    REQUIRE(channels.mmio_read(TagClassId::SPU_MBox_Status) == 0x000400);
    REQUIRE(channels.readCount(SPU_WrOutMbox) == 1);
    channels.write(SPU_WrOutMbox, 17);
    REQUIRE(channels.readCount(SPU_WrOutMbox) == 0);
    
    // read from mbox from ppu
    REQUIRE(channels.mmio_read(TagClassId::SPU_MBox_Status) == 0x000401);
    REQUIRE(channels.mmio_readCount(TagClassId::SPU_Out_MBox) == 1);
    REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 17);
    REQUIRE(channels.mmio_readCount(TagClassId::SPU_Out_MBox) == 0);
    REQUIRE(channels.mmio_read(TagClassId::SPU_MBox_Status) == 0x000400);
    
    // write again from spu
    channels.write(SPU_WrOutMbox, 13);
    
    // read again from ppu
    REQUIRE(channels.mmio_read(TagClassId::SPU_MBox_Status) == 0x000401);
    REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 13);
}

TEST_CASE("spuchannels_mailbox_two_threads_blocking") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    boost::thread spu([&] {
        channels.write(SPU_WrOutMbox, 1);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        channels.write(SPU_WrOutMbox, 2);
        channels.write(SPU_WrOutMbox, 3);
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        channels.write(SPU_WrOutMbox, 4);
    });
    
    boost::thread ppu([&] {
        while ((channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff) == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 1);
        while ((channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff) == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 2);
        while ((channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff) == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 3);
        while ((channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff) == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 4);
    });
    
    spu.join();
    ppu.join();
}

TEST_CASE("spuchannels_event_basic") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
        
    boost::thread spu([&] {
        channels.write(SPU_WrEventMask, 0b1000);
        REQUIRE(channels.read(SPU_RdEventMask) == 0b1000);
        REQUIRE(channels.read(SPU_RdEventStat) == 0b1000); // the same result until acknowledged
        REQUIRE(channels.read(SPU_RdEventStat) == 0b1000);
        REQUIRE(channels.read(SPU_RdEventStat) == 0b1000);
        channels.write(SPU_WrEventAck, 0b1000);
        REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
        channels.write(SPU_WrEventMask, 0b0111);
        REQUIRE(channels.readCount(SPU_RdEventStat) == 1);
        REQUIRE(channels.read(SPU_RdEventStat) == 0b111);
        channels.write(SPU_WrEventAck, 0b0111);
        REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    });
    
    boost::thread ppu([&] {
        channels.setEvent(0b110111);
        channels.setEvent(0b001000);
    });
    
    spu.join();
    ppu.join();
}

TEST_CASE("spuchannels_event_reservation") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);
    
    ReservationGranule granule;
    g_state.granule = &granule;
        
    channels.write(SPU_WrEventMask, 1u << 10);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // the same thread destroys the reservation but doesn't raise the event
    uint32_t val = 0;
    mm.writeCond<4>(0x10000, &val);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // there is no reservation, and therefore no event is raised
    boost::thread ppu([&] {
        mm.store32(0x10000, 0, g_state.granule);
    });
    ppu.join();
    
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // create a reservation
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // another thread destroys the reservation and raises the event
    boost::thread ppu1([&] {
        mm.store32(0x10000, 0, g_state.granule);
    });
    ppu1.join();
    
    REQUIRE(channels.readCount(SPU_RdEventStat) == 1);
    REQUIRE(channels.read(SPU_RdEventStat) == 1u << 10);
}

TEST_CASE("spuchannels_event_reservation_thread_can_only_have_one_reservation_at_a_time") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    ReservationGranule granule;
    g_state.granule = &granule;
    
    mm.mark(0x10000, 0x1000, false, "");
    mm.mark(0x20000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);
    mm.setMemory(0x20000, 0, 1000);
        
    channels.write(SPU_WrEventMask, 1u << 10);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x20000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
 
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_PUTLLC_CMD);
    REQUIRE((channels.read(MFC_RdAtomicStat) & 1) == 1); // failure
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x20000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_PUTLLC_CMD);
    REQUIRE((channels.read(MFC_RdAtomicStat) & 1) == 0); // success
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x20000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_PUTLLC_CMD);
    REQUIRE((channels.read(MFC_RdAtomicStat) & 1) == 1); // failure
}

TEST_CASE("spuchannels_event_reservation_2threads") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    channels.write(SPU_WrEventMask, (unsigned)MfcEvent::MFC_LLR_LOST_EVENT);
    
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);

    std::atomic<bool> th1flag = false, th2flag = false;
    
    bool f1 = false, s1 = false, f2 = false, s2 = false;
    
    // th1 creates a reservation
    boost::thread th1([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        
        channels.write(MFC_EAH, 0);
        channels.write(MFC_EAL, 0x10000);
        channels.write(MFC_LSA, 0x300);
        channels.write(MFC_Size, 0x80);
        channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
        f1 = channels.readCount(SPU_RdEventStat) == 0;
        
        th1flag = true;
        
        s1 = channels.read(SPU_RdEventStat) == (unsigned)MfcEvent::MFC_LLR_LOST_EVENT;
    });
    
    while (!th1flag) ;
    
    TestSPUChannelsThread thread2;
    SPUChannels channels2(&mm, &thread2);
    channels2.write(SPU_WrEventMask, (unsigned)MfcEvent::MFC_LLR_LOST_EVENT);
    
    // th2 creates its own reservation and this doesn't destroy the reservation of th1
    boost::thread th2([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        
        channels2.write(MFC_EAH, 0);
        channels2.write(MFC_EAL, 0x10000);
        channels2.write(MFC_LSA, 0x300);
        channels2.write(MFC_Size, 0x80);
        channels2.write(MFC_Cmd, MFC_GETLLAR_CMD);
        f2 = channels2.readCount(SPU_RdEventStat) == 0;
        
        th2flag = true;
        
        s2 = channels2.read(SPU_RdEventStat) == (unsigned)MfcEvent::MFC_LLR_LOST_EVENT;
    });
    
    while (!th2flag) ;
    
    // a third thread removes reservation, th1 and th2 lose their reservations
    boost::thread ppu([&] {
        mm.store32(0x10000, 0, g_state.granule);
    });
    
    ppu.join();
    th1.join();
    th2.join();
    
    REQUIRE(f1);
    REQUIRE(f2);
    REQUIRE(s1);
    REQUIRE(s2);
}

TEST_CASE("spuchannels_event_reservation_putllc_should_not_raise_event") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);
    
    ReservationGranule granule;
    g_state.granule = &granule;
        
    channels.write(SPU_WrEventMask, 1u << 10);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
 
    // the same thread destroys the reservation but doesn't raise the event
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_PUTLLC_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    REQUIRE((channels.read(MFC_RdAtomicStat) & 1) == 0); // success
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_PUTLLC_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    REQUIRE((channels.read(MFC_RdAtomicStat) & 1) == 1); // failure
    
    // there is no reservation, and therefore no event is raised
    boost::thread ppu([&] {
        mm.store32(0x10000, 0, g_state.granule);
    });
    ppu.join();
    
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // setup a reservation
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // another thread destroys the reservation and raises the event
    boost::thread ppu1([&] {
        mm.store32(0x10000, 0, g_state.granule);
    });
    ppu1.join();
    
    REQUIRE(channels.readCount(SPU_RdEventStat) == 1);
    REQUIRE(channels.read(SPU_RdEventStat) == 1u << 10);
}

TEST_CASE("spuchannels_reservation_ticket_lock") {
    MainMemory mm;
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);
    
    TestSPUChannelsThread tth1, tth2, tth3;
    SPUChannels ch1(&mm, &tth1);
    SPUChannels ch2(&mm, &tth2);
    SPUChannels ch3(&mm, &tth3);
    
    auto atomic_increment = [&](SPUChannels* channels, TestSPUChannelsThread* th, int index) {
        auto ptr = (big_uint16_t*)th->ls(0x300 + index * 2);
        uint32_t val;
        do {
            channels->write(MFC_EAH, 0);
            channels->write(MFC_EAL, 0x10000);
            channels->write(MFC_LSA, 0x300);
            channels->write(MFC_Size, 0x80);
            channels->write(MFC_Cmd, MFC_GETLLAR_CMD);
            
            val = *ptr;
            (*ptr)++;
            
            channels->write(MFC_EAH, 0);
            channels->write(MFC_EAL, 0x10000);
            channels->write(MFC_LSA, 0x300);
            channels->write(MFC_Size, 0x80);
            channels->write(MFC_Cmd, MFC_PUTLLC_CMD);
        } while ((channels->read(MFC_RdAtomicStat) & 1) != 0);
        return val;
    };
    
    auto load_current = [&](SPUChannels* channels, TestSPUChannelsThread* th) {
        channels->write(MFC_EAH, 0);
        channels->write(MFC_EAL, 0x10000);
        channels->write(MFC_LSA, 0x300);
        channels->write(MFC_Size, 0x80);
        channels->write(MFC_Cmd, MFC_GETLLAR_CMD);
        return *(big_uint16_t*)th->ls(0x300);
    };
    
    auto current = 0;
    auto next = 1;
    
    auto acquire = [&](SPUChannels* channels, TestSPUChannelsThread* th) {
        auto ticket = atomic_increment(channels, th, next);
        while (load_current(channels, th) != ticket) ;
    };
    
    auto release = [&](SPUChannels* channels, TestSPUChannelsThread* th) {
        atomic_increment(channels, th, current);
    };
    
    int count = 0;
    
    boost::thread th1([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        log_set_thread_name("th1");
        for (int i = 0; i < 5000; ++i) {
            acquire(&ch1, &tth1);
            count++;
            release(&ch1, &tth1);
        }
    });
    
    boost::thread th2([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        log_set_thread_name("th2");
        for (int i = 0; i < 5000; ++i) {
            acquire(&ch2, &tth2);
            count++;
            release(&ch2, &tth2);
        }
    });
    
    boost::thread th3([&] {
        ReservationGranule granule;
        g_state.granule = &granule;
        for (int i = 0; i < 5000; ++i) {
            acquire(&ch3, &tth3);
            count++;
            release(&ch3, &tth3);
        }
    });
    
    th1.join();
    th2.join();
    th3.join();
    
    REQUIRE(count == 15000);
}

TEST_CASE("spuchannels_signal_basic") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    REQUIRE(channels.readCount(SPU_RdSigNotify1) == 0);
    REQUIRE(channels.readCount(SPU_RdSigNotify2) == 0);
    
    // mmio doesn't reset the signal
    channels.mmio_write(SPU_Sig_Notify_1, 0b1010);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_1) == 0b1010);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_1) == 0b1010);
    channels.mmio_write(SPU_Sig_Notify_2, 0b1110);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_2) == 0b1110);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_2) == 0b1110);
    
    // spu resets the signal
    REQUIRE(channels.read(SPU_RdSigNotify1) == 0b1010);
    REQUIRE(channels.read(SPU_RdSigNotify2) == 0b1110);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_1) == 0);
    REQUIRE(channels.mmio_read(SPU_Sig_Notify_2) == 0);
    
    boost::thread spu([&] {
        REQUIRE(channels.read(SPU_RdSigNotify1) == 0b110100);
        REQUIRE(channels.read(SPU_RdSigNotify2) == 0b110111);
    });
    
    boost::thread ppu([&] {
        channels.mmio_write(SPU_Sig_Notify_2, 0b110111);
        channels.mmio_write(SPU_Sig_Notify_1, 0b110100);
    });
    
    spu.join();
    ppu.join();
}

TEST_CASE("spuchannels_dma_updates") {
    MainMemory mm;
    TestSPUChannelsThread thread;
    SPUChannels channels(&mm, &thread);
    
    mm.mark(0x10000, 0x1000, false, "");
    mm.setMemory(0x10000, 0, 1000);
    
    auto mask1 = 0x10u;
    auto mask2 = 0x01u;
    
    channels.write(MFC_WrTagMask, 0x13);
    REQUIRE(channels.read(MFC_RdTagMask) == 0x13);
    
    // update all
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_TagID, 4);
    channels.write(MFC_Cmd, MFC_GET_CMD);
    
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_TagID, 0);
    channels.write(MFC_Cmd, MFC_GET_CMD);
    
    channels.write(MFC_WrTagMask, mask1 | mask2);
    channels.write(MFC_WrTagUpdate, (unsigned)MfcTag::MFC_TAG_UPDATE_ALL);
    REQUIRE( channels.readCount(MFC_WrTagUpdate) != 0 );
    REQUIRE( channels.read(MFC_RdTagStat) == (mask1 | mask2) );
}
