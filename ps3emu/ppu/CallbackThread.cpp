#include "CallbackThread.h"

#include "../Process.h"
#include "../ELFLoader.h"
#include "../MainMemory.h"
#include "../InternalMemoryManager.h"
#include "../state.h"
#include "ppu_dasm_forms.h"
#include <boost/endian/arithmetic.hpp>
#include <utility>

using namespace boost::endian;

struct ThreadBody {
    fdescr descr;
    big_uint32_t ncall;
    big_uint32_t bl;
    big_uint64_t thread;
};

std::future<void> CallbackThread::schedule(std::vector<uint64_t> args,
                                           uint32_t toc,
                                           uint32_t ea) {
    CallbackInfo info{false, args, toc, ea, std::make_shared<std::promise<void>>()};
    auto promise = info.promise->get_future();
    _queue.send(std::move(info));
    return promise;
}

CallbackThread::CallbackThread(Process* proc) {
    _lastCallback.terminate = true;
    
    uint32_t index;
    findNCallEntry(calcFnid("callbackThreadQueueWait"), index);
    
    uint32_t bodyEa;
    auto body = g_state.memalloc->internalAlloc<2, ThreadBody>(&bodyEa);

    body->ncall = (1 << 26) | index;
    
    IForm bl {0};
    bl.OPCD.set(18);
    bl.LI.set(-1);
    bl.AA.set(0);
    bl.LK.set(1);
    
    body->bl = bl.u32;
    body->descr.va = bodyEa + offsetof(ThreadBody, bl);
    body->descr.tocBase = 0;
    body->thread = (uint64_t)this;
    
    proc->createThread(0x1000, bodyEa, 0);
}

uint64_t callbackThreadQueueWait(PPUThread* ppuThread) {
    auto lr = ppuThread->getLR();
    auto thread = (CallbackThread*)g_state.mm->load<8>(lr);
    
    if (!thread->_lastCallback.terminate)
        thread->_lastCallback.promise->set_value();
    
    auto info = thread->_queue.receive(0);
    
    if (info.terminate)
        throw ThreadFinishedException(0);
    
    ppuThread->setNIP(info.ea);
    ppuThread->setGPR(2, info.toc);
    ppuThread->setLR(lr - 4);
    int i = 3;
    for (auto arg : info.args) {
        ppuThread->setGPR(i, arg);
        i++;
    }
    
    thread->_lastCallback = std::move(info);
    
    return ppuThread->getGPR(3);
}

void CallbackThread::terminate() {
    _queue.send({true, {}, 0, 0, nullptr});
}
