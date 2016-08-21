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
