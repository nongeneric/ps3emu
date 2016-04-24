#include "InterruptPPUThread.h"

#include "../spu/SPUThread.h"
#include "../ELFLoader.h"
#include "../Process.h"
#include "../MainMemory.h"
#include "../log.h"

void InterruptPPUThread::innerLoop() {
    log_set_thread_name("ppu_interrupt");
    for (;;) {
        auto ev = _queue.receive(0);
        if (ev.type == InterruptType::Disestablish)
            return;
        if (_mask2 & ev.status) {
            fdescr entryDescr;
            proc()->mm()->readMemory(_entry, &entryDescr, sizeof(fdescr));
    
            setNIP(entryDescr.va);
            setGPR(2, entryDescr.tocBase);
            setGPR(3, _arg);
            PPUThread::innerLoop();
        }
    }
}

void InterruptPPUThread::setEntry(uint32_t entry) {
    _entry = entry;
}

void InterruptPPUThread::disestablish() {
    _queue.send({ InterruptType::Disestablish, 0 });
}

void InterruptPPUThread::establish(SPUThread* thread) {
    thread->setInterruptHandler([=] {
        _queue.send({ InterruptType::Spu, thread->getStatus() });
    });
}

void InterruptPPUThread::setMask2(uint32_t mask) {
    _mask2 = mask;
}

void InterruptPPUThread::setArg(uint64_t arg) {
    _arg = arg;
}

InterruptPPUThread::InterruptPPUThread(
    Process* proc, std::function<void(PPUThread*, PPUThreadEvent)> eventHandler)
    : PPUThread(proc, eventHandler, false), _mask2(0) {}

    
