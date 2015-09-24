#include "Rsx.h"
#include "LocalMemory.h"

#include <boost/log/trivial.hpp>

uint32_t Rsx::getGet() {
    boost::unique_lock<boost::mutex> lock(_mutex);
    return _get;
}

void Rsx::setPut(uint32_t put) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _put = put;
}

void Rsx::loop() {
    boost::unique_lock<boost::mutex> lock(_mutex);
    for (;;) {
        _cv.wait(lock);
        if (_shutdown)
            return;
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
    auto method = _localMemory->load(get);
    return method;
}

void Rsx::setRegs(emu::CellGcmControl* regs) {
    boost::unique_lock<boost::mutex> lock(_mutex);
    _put = regs->put;
    _get = regs->get;
    _cv.notify_all();
}

Rsx::Rsx(LocalMemory* localMemory) : _localMemory(localMemory) {
    _thread.reset(new boost::thread([=]{ loop(); }));    
}

TargetProxyCellGcmContextData Rsx::getCurrentContext(uint32_t currentContextOffset) {
    TargetProxyCellGcmContextData context;
    context.begin = _localMemory->load(currentContextOffset);
    context.end = _localMemory->load(currentContextOffset + 4);
    context.current = _localMemory->load(currentContextOffset + 8);
    context.callback = _localMemory->load(currentContextOffset + 12);
    return context;
}

void Rsx::updateCurrentContext(TargetProxyCellGcmContextData* context, uint32_t currentContextOffset) {
    _localMemory->store(currentContextOffset, context->begin);
    _localMemory->store(currentContextOffset + 4, context->end);
    _localMemory->store(currentContextOffset + 8, context->current);
    _localMemory->store(currentContextOffset + 12, context->callback);
    setPut(context->end);
}

void Rsx::shutdown() {
    if (_shutdown)
        return;
    boost::unique_lock<boost::mutex> lock(_mutex);
    _shutdown = true;
    _cv.notify_all();
    _thread->join();
}
