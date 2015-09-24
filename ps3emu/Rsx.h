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

struct TargetProxyCellGcmContextData {
    uint32_t begin;
    uint32_t end;
    uint32_t current;
    uint32_t callback;
};

class LocalMemory;
class Rsx {
    uint32_t _get;
    uint32_t _put;
    bool _shutdown = false;
    LocalMemory* _localMemory;
    boost::mutex _mutex;
    boost::condition_variable _cv;
    std::unique_ptr<boost::thread> _thread;
    uint32_t interpret(uint32_t get);
    void loop();
    void setPut(uint32_t put);
public:
    Rsx(LocalMemory* localMemory);
    void shutdown();
    void setRegs(emu::CellGcmControl* regs);
    uint32_t getGet();
    void updateCurrentContext(TargetProxyCellGcmContextData* context, uint32_t currentContextOffset);
    TargetProxyCellGcmContextData getCurrentContext(uint32_t currentContextOffset);
    inline LocalMemory* localMemory() {
        return _localMemory;
    }
};