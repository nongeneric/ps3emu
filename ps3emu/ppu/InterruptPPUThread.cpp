#include "InterruptPPUThread.h"

#include "../spu/SPUThread.h"
#include "../ELFLoader.h"
#include "../Process.h"
#include "../MainMemory.h"
#include "../log.h"
#include "../state.h"
#include <assert.h>

void InterruptPPUThread::innerLoop() {
    log_set_thread_name("ppu_interrupt");
    for (;;) {
        auto ev = _queue.receive(0);
        if (ev.type == InterruptType::Disestablish)
            return;
        fdescr entryDescr;
        g_state.mm->readMemory(_entry, &entryDescr, sizeof(fdescr));

        setNIP(entryDescr.va);
        setGPR(2, entryDescr.tocBase);
        setGPR(3, _arg);
        PPUThread::innerLoop();
    }
}

void InterruptPPUThread::setEntry(uint32_t entry) {
    _entry = entry;
}

void InterruptPPUThread::disestablish() {
    _queue.send({ InterruptType::Disestablish });
}

void InterruptPPUThread::establish(SPUThread* thread) {
    _establishedThread = thread;
    thread->setInterruptHandler(_mask2, [this] {
        _queue.send({ InterruptType::Spu });
    });
}

void InterruptPPUThread::setMask2(uint32_t mask) {
    _mask2 = mask;
    establish(_establishedThread);
}

void InterruptPPUThread::setArg(uint64_t arg) {
    _arg = arg;
}

InterruptPPUThread::InterruptPPUThread(
    std::function<void(PPUThread*, PPUThreadEvent, std::any)> eventHandler)
    : PPUThread(eventHandler, false), _mask2(0) {}

    
