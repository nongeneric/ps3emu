#pragma once

#include "constants.h"
#include "../libs/graphics/gcm.h"
#include <boost/thread.hpp>
#include <boost/endian/arithmetic.hpp>
#include <memory>

namespace emu {

typedef struct {
    boost::endian::big_uint32_t put;
    boost::endian::big_uint32_t get;
    boost::endian::big_uint32_t ref;
} CellGcmControl;

}

class PPU;
class Rsx {
    uint32_t _get;
    uint32_t _put;
    bool _shutdown = false;
    PPU* _ppu;
    boost::mutex _mutex;
    boost::condition_variable _cv;
    std::unique_ptr<boost::thread> _thread;
    uint32_t interpret(uint32_t get);
    void loop();
public:
    Rsx(PPU* ppu);
    void shutdown();
    void setPut(uint32_t put);
    void setRegs(emu::CellGcmControl* regs);
};