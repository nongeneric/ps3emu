#include "queue.h"

#include "../ConcurrentQueue.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/ContentManager.h"
#include "../spu/sysSpu.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/Process.h"
#include <boost/thread.hpp>
#include <algorithm>

namespace {    
    using IQueue = IConcurrentQueue<sys_event_t>;
    
    struct queue_port_t {
        uint64_t name;
        int32_t type;
        IQueue* queue = nullptr;
    };

    ThreadSafeIDMap<sys_event_queue_t, std::shared_ptr<IQueue>, QueueIdBase> queues;
    ThreadSafeIDMap<sys_event_port_t, std::shared_ptr<queue_port_t>> ports;
}

#define   SYS_SYNC_FIFO                     0x00001
#define   SYS_SYNC_PRIORITY                 0x00002
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL   0x01
#define SYS_SPU_THREAD_EVENT_USER 0x1

int32_t sys_event_queue_create(sys_event_queue_t* equeue_id,
                               sys_event_queue_attribute_t* attr,
                               sys_ipc_key_t event_queue_key,
                               uint32_t size) {
    assert(1 <= size && size < 128);
    // TODO: handle unique key
    //assert(event_queue_key == SYS_EVENT_QUEUE_LOCAL);
    assert(attr->attr_protocol == SYS_SYNC_PRIORITY ||
           attr->attr_protocol == SYS_SYNC_FIFO);
    std::shared_ptr<IQueue> queue;
    if (attr->attr_protocol == SYS_SYNC_PRIORITY) {
        queue.reset(new ConcurrentPriorityQueue<sys_event_t>);
    } else {
        queue.reset(new ConcurrentFifoQueue<sys_event_t>);
    }
    *equeue_id = queues.create(std::move(queue));
    INFO(libs) << ssnprintf("sys_event_queue_create(id = %d)", *equeue_id);
    return CELL_OK;
}

int32_t sys_event_queue_destroy(sys_event_queue_t equeue_id, int32_t mode) {
    LOG << __FUNCTION__;
    assert(!mode);
    queues.destroy(equeue_id);
    return CELL_OK;
}

int32_t sys_event_queue_receive(sys_event_queue_t equeue_id,
                                uint32_t unused,
                                usecond_t timeout,
                                PPUThread* th)
{
    // TODO: handle timeout
    //assert(timeout == 0);
    INFO(libs) << ssnprintf("sys_event_queue_receive(%d)", equeue_id);
    auto queue = queues.get(equeue_id);
    auto event = queue->receive(th->priority());
    th->setGPR(4, event.source);
    th->setGPR(5, event.data1);
    th->setGPR(6, event.data2);
    th->setGPR(7, event.data3);
    INFO(libs) << ssnprintf("completed sys_event_queue_receive(%d)", equeue_id);
    return CELL_OK;
}

int32_t sys_event_queue_tryreceive(sys_event_queue_t equeue_id,
                                   uint32_t event_array,
                                   int32_t size,
                                   big_uint32_t *number,
                                   PPUThread* th)
{
    INFO(libs) << ssnprintf("sys_event_queue_tryreceive(%d)", equeue_id);
    auto queue = queues.get(equeue_id);
    std::vector<sys_event_t> vec(size);
    size_t num;
    queue->tryReceive(&vec[0], vec.size(), &num);
    *number = num;
    g_state.mm->writeMemory(event_array, &vec[0], sizeof(sys_event_t) * *number);
    INFO(libs) << ssnprintf("completed sys_event_queue_tryreceive(%d)", equeue_id);
    return CELL_OK;
}

int32_t sys_event_port_create(sys_event_port_t* eport_id, int32_t port_type, uint64_t name) {
    auto port = std::make_shared<queue_port_t>();
    port->name = name;
    port->type = port_type;
    *eport_id = ports.create(port);
    INFO(libs) << ssnprintf("sys_event_port_create(id = %d, name = %016llx)", *eport_id, name);
    return CELL_OK;
}

int32_t sys_event_port_connect_local(sys_event_port_t event_port_id, 
                                     sys_event_queue_t event_queue_id)
{
    INFO(libs) << ssnprintf("sys_event_port_connect_local(port = %d, queue = %d)",
                            event_port_id, event_queue_id);
    assert(ports.get(event_port_id)->type == SYS_EVENT_PORT_LOCAL);
    ports.get(event_port_id)->queue = queues.get(event_queue_id).get();
    return CELL_OK;
}

int32_t sys_event_port_send(sys_event_port_t eport_id,
                            uint64_t data1,
                            uint64_t data2,
                            uint64_t data3)
{
    INFO(libs) << ssnprintf("sys_event_port_send(port = %d, data = %016llx, %016llx, %016llx)",
                            eport_id, data1, data2, data3);
    auto port = ports.get(eport_id);
    port->queue->send({ port->name, data1, data2, data3 });
    return CELL_OK;
}

int32_t sys_event_queue_drain(sys_event_queue_t equeue_id) {
    queues.get(equeue_id)->drain();
    return CELL_OK;
}

int32_t sys_event_port_disconnect(sys_event_port_t event_port_id) {
    ports.get(event_port_id)->queue = nullptr;
    return CELL_OK;
}

int32_t sys_event_port_destroy(sys_event_port_t eport_id) {
    ports.destroy(eport_id);
    return CELL_OK;
}

int32_t sys_spu_thread_connect_event(uint32_t thread_id,
                                     sys_event_queue_t eq,
                                     sys_event_type_t et,
                                     uint8_t spup,
                                     Process* proc) {
    INFO(libs) << ssnprintf(
        "sys_spu_thread_connect_event(thread = %d, queue = %d, spup = %02x)", thread_id, eq, spup);
    assert(et == SYS_SPU_THREAD_EVENT_USER);
    auto thread = proc->getSpuThread(thread_id);
    auto queue = queues.get(eq);
    thread->connectOrBindQueue(queue, spup);
    return CELL_OK;
}

int32_t sys_spu_thread_bind_queue(uint32_t thread_id,
                                  sys_event_queue_t spuq,
                                  uint32_t spuq_num,
                                  Process* proc) {
    INFO(libs) << ssnprintf(
        "sys_spu_thread_bind_queue(thread = %d, queue = %d, spuq = %02x)", thread_id, spuq, spuq_num);
    auto thread = proc->getSpuThread(thread_id);
    auto queue = queues.get(spuq);
    thread->connectOrBindQueue(queue, spuq_num);
    return CELL_OK;
}

int32_t sys_spu_thread_group_connect_event_all_threads(uint32_t group_id,
                                                       sys_event_queue_t eq,
                                                       uint64_t req,
                                                       uint8_t* spup,
                                                       Process* proc) {
    INFO(libs) << ssnprintf("sys_spu_thread_group_connect_event_all_threads"
                            "(group = %d, queue = %d, req = %016llx)", 
                            group_id, eq, req);
    auto group = findThreadGroup(group_id);
    std::vector<SPUThread*> threads;
    std::transform(begin(group->threads), end(group->threads), std::back_inserter(threads), [=](auto id) {
        return proc->getSpuThread(id);
    });
    auto queue = queues.get(eq);
    for (auto i = 0u; i < 64; ++i) {
        if (((1ull << i) & req) == 0)
            continue;
        auto available = std::all_of(begin(threads), end(threads), [=](auto& th) {
            return th->isAvailableQueuePort(i);
        });
        if (available) {
            for (auto& th : threads) {
                th->connectOrBindQueue(queue, i);
            }
            *spup = i;
            INFO(libs) << ssnprintf("connected to spup %d", i);
            return CELL_OK;
        }
    }
    assert(false);
    return -1;
}
