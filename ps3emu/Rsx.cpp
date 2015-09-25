#include "Rsx.h"

#include "PPU.h"
#include <boost/log/trivial.hpp>

void Rsx::setPut(uint32_t put) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _put = put;
}

void Rsx::loop() {
    BOOST_LOG_TRIVIAL(trace) << "rsx loop started, waiting for updates";
    boost::unique_lock<boost::mutex> lock(_mutex);
    for (;;) {
        _cv.wait(lock);
        BOOST_LOG_TRIVIAL(trace) << "rsx loop update recieved";
        if (_shutdown) {
            BOOST_LOG_TRIVIAL(trace) << "rsx loop shutdown";
            return;
        }
        while (_get != _put) {
            auto get = _get;
            lock.unlock();
            auto len = interpret(get);
            lock.lock();
            _get += len;
        }
    }
}

uint32_t Rsx::interpret(uint32_t get) {
    BOOST_LOG_TRIVIAL(trace) << "rsx instruction at " << get;
    auto method = _ppu->load<4>(GcmLocalMemoryBase + get);
    return method;
}

void Rsx::setRegs(emu::CellGcmControl* regs) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _put = regs->put;
    _get = regs->get;
    _cv.notify_all();
}

Rsx::Rsx(PPU* ppu) : _ppu(ppu) {
    _thread.reset(new boost::thread([=]{ loop(); }));    
}

void Rsx::shutdown() {
    assert(!_shutdown);
    BOOST_LOG_TRIVIAL(trace) << "waiting for rsx to shutdown";
    {
        boost::unique_lock<boost::mutex> lock(_mutex);
        _shutdown = true;
    }
    _cv.notify_all();
    _thread->join();
}
