#include "CallbackThread.h"

#include "../Process.h"
#include "../ELFLoader.h"
#include "../MainMemory.h"
#include "../InternalMemoryManager.h"
#include "ppu_dasm_forms.h"
#include <boost/endian/arithmetic.hpp>
#include <utility>

using namespace boost::endian;

struct ThreadBody {
    fdescr descr;
    big_uint32_t ncall;
    big_uint32_t br;
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
    auto internal = proc->internalMemoryManager();
    
    uint32_t index;
    findNCallEntry(calcFnid("callbackThreadQueueWait"), index);
    
    uint32_t bodyEa;
    auto body = internal->internalAlloc<2, ThreadBody>(&bodyEa);
    body->ncall = (1 << 26) | index;
    
    IForm br {0};
    br.OPCD.set(18);
    br.LI.set(-4);
    br.AA.set(0);
    br.LK.set(0);
    
    body->br = br.u32;
    body->descr.va = bodyEa + offsetof(ThreadBody, ncall);
    body->descr.tocBase = 0;
    
    proc->createThread(0x1000, bodyEa, (uint64_t)this);
}

int callbackThreadQueueWait(uint64_t threadPtr, PPUThread* ppuThread) {
    auto thread = (CallbackThread*)threadPtr;
    
    if (!thread->_lastCallback.terminate)
        thread->_lastCallback.promise->set_value();
    
    auto info = thread->_queue.receive(0);
    
    if (info.terminate)
        throw ThreadFinishedException(0);
    
    ppuThread->setNIP(info.ea);
    ppuThread->setGPR(2, info.toc);
    int i = 3;
    for (auto arg : info.args) {
        ppuThread->setGPR(i, arg);
        i++;
    }
    return 0;
}

void CallbackThread::terminate() {
    _queue.send({true, {}, 0, 0, nullptr});
}