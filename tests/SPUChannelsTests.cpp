#include <catch.hpp>
#include "ps3emu/MainMemory.h"
#include "ps3emu/spu/SPUChannels.h"
#include "ps3emu/spu/SPUThread.h"
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

class TestSPUChannelsThread : public ISPUChannelsThread {
public:
    bool _run = false;
    uint32_t _nip = 0;
    std::array<uint8_t, LocalStorageSize> _ls;
    void run() override { _run = true; }
    void setNip(uint32_t nip) override { _nip = nip; }
    uint8_t* ls() override { return &_ls[0]; }
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
        while (channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 1);
        while (channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 2);
        while (channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff == 0) ;
        REQUIRE(channels.mmio_read(TagClassId::SPU_Out_MBox) == 3);
        while (channels.mmio_read(TagClassId::SPU_MBox_Status) & 0xff == 0) ;
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
    
    mm.setMemory(0x10000, 0, 1000, true);
        
    channels.write(SPU_WrEventMask, 1u << 10);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    channels.write(MFC_EAH, 0);
    channels.write(MFC_EAL, 0x10000);
    channels.write(MFC_LSA, 0x300);
    channels.write(MFC_Size, 0x80);
    channels.write(MFC_Cmd, MFC_GETLLAR_CMD);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
 
    // the same thread destroys the reservation but doesn't raise the event
    mm.store<4>(0x10000, 0);
    REQUIRE(channels.readCount(SPU_RdEventStat) == 0);
    
    // there is no reservation, and therefore no event is raised
    boost::thread ppu([&] {
        mm.store<4>(0x10000, 0);
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
        mm.store<4>(0x10000, 0);
    });
    ppu1.join();
    
    REQUIRE(channels.readCount(SPU_RdEventStat) == 1);
    REQUIRE(channels.read(SPU_RdEventStat) == 1u << 10);
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
