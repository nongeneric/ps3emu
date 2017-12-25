#include "CallbackThread.h"

#include "../Process.h"
#include "../MainMemory.h"
#include "../InternalMemoryManager.h"
#include "../state.h"
#include "ppu_dasm_forms.h"
#include <boost/endian/arithmetic.hpp>
#include <utility>

using namespace boost::endian;

struct CallbackThreadInitInfo {
    big_uint64_t threadId;
    char name[16];
    fdescr descr;
    big_uint32_t ncall;
    big_uint32_t bl;
    big_uint64_t thread;
};

std::future<void> CallbackThread::schedule(std::vector<uint64_t> args,
                                           uint32_t toc,
                                           uint32_t ea) {
    CallbackInfo info{false, args, toc, ea, std::make_shared<std::promise<void>>()};
    auto future = info.promise->get_future();
    if (_terminated) {
        info.promise->set_value();
        return future;
    }
    _queue.enqueue(std::move(info));
    return future;
}

CallbackThread::CallbackThread() : _queue(1) {
    _lastCallback.terminate = true;
    
    uint32_t index;
    findNCallEntry(calcFnid("callbackThreadQueueWait"), index);
    
    _initInfo = g_state.memalloc->internalAlloc<4, CallbackThreadInitInfo>(&_initInfoVa);
    _initInfo->ncall = (NCALL_OPCODE << 26) | index;
    
    IForm bl {0};
    bl.OPCD.set(18);
    bl.LI.set(-1);
    bl.AA.set(0);
    bl.LK.set(1);
    
    _initInfo->bl = bl.u32;
    _initInfo->descr.va = _initInfoVa + offsetof(CallbackThreadInitInfo, bl);
    _initInfo->descr.tocBase = 0;
    _initInfo->thread = (uint64_t)this;
}

uint64_t callbackThreadQueueWait(PPUThread* ppuThread) {
    auto lr = ppuThread->getLR();
    auto thread = (CallbackThread*)g_state.mm->load64(lr);
    
    if (!thread->_lastCallback.terminate)
        thread->_lastCallback.promise->set_value();
    
    auto info = thread->_queue.dequeue();
    
    if (info.terminate) {
        if (thread->_queue.size()) {
            WARNING(libs) << ssnprintf("callback has pending events");
        }
        throw ThreadFinishedException(0);
    }
    
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
    _terminated = true;
    _queue.enqueue({true, {}, 0, 0, nullptr});
}

uint64_t CallbackThread::id() {
    return _initInfo->threadId;
}

void CallbackThread::ps3callInit(std::string_view name, boost::context::continuation* sink) {
    strncpy(_initInfo->name, begin(name), sizeof(CallbackThreadInitInfo::name) - 1);
    auto& segments = g_state.proc->getSegments();
    assert(segments.size() > 2);
    auto& lv2segment = segments[2];
    auto func = g_state.proc->findExport(lv2segment.elf.get(), calcFnid("sys_ppu_thread_create"));
    assert(!!func);
    std::initializer_list<uint64_t> args = {
        _initInfoVa + offsetof(CallbackThreadInitInfo, threadId),
        _initInfoVa + offsetof(CallbackThreadInitInfo, descr),
        0, // arg
        1000, // priority
        0x1000, // stack size
        0, // flags
        _initInfoVa + offsetof(CallbackThreadInitInfo, name)
    };
    g_state.th->ps3call(*func, args, sink);
}
