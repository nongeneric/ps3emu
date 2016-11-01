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
    
    struct queue_info {
        std::shared_ptr<IQueue> queue;
        std::string name;
    };
    
    struct queue_port_t {
        uint64_t name;
        int32_t type;
        std::shared_ptr<IQueue> queue;
    };

    ThreadSafeIDMap<sys_event_queue_t, std::shared_ptr<queue_info>, QueueIdBase> queues;
    ThreadSafeIDMap<sys_event_port_t, std::shared_ptr<queue_port_t>> ports;
}

#define SYS_SYNC_FIFO 0x00001
#define SYS_SYNC_PRIORITY 0x00002
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL 0x01
#define SYS_SPU_THREAD_EVENT_USER 0x1

bool isPrintStr(char* str, int len) {
    int i = 0;
    for (; i < len; ++i) {
        if (!str[i])
            break;
        if (!isprint(str[i]))
            return false;
    }
    for (; i < len; ++i) {
        if (str[i] != 0)
            return false;
    }
    return true;
}

std::string printName(char* str, int len) {
    assert(len == 8);
    if (isPrintStr(str, len)) {
        return std::string(str);
    }
    return ssnprintf("%016llx", *(uint64_t*)str);
}

int32_t sys_event_queue_create(sys_event_queue_t* equeue_id,
                               sys_event_queue_attribute_t* attr,
                               sys_ipc_key_t event_queue_key,
                               uint32_t size) {
    assert(1 <= size && size < 128);
    // TODO: handle unique key
    //assert(event_queue_key == SYS_EVENT_QUEUE_LOCAL);
    assert(attr->attr_protocol == SYS_SYNC_PRIORITY ||
           attr->attr_protocol == SYS_SYNC_FIFO);
    auto info = std::make_shared<queue_info>();
    if (attr->attr_protocol == SYS_SYNC_PRIORITY) {
        info->queue.reset(new ConcurrentPriorityQueue<sys_event_t>(size));
    } else {
        info->queue.reset(new ConcurrentFifoQueue<sys_event_t>(size));
    }
    auto name = printName(attr->name, SYS_SYNC_NAME_SIZE);
    info->name = name;
    *equeue_id = queues.create(std::move(info));
    INFO(libs) << ssnprintf("sys_event_queue_create(id = %x, event_queue_key = %llx, "
                            "size = %d, name = %s, %s)",
                            *equeue_id,
                            event_queue_key,
                            size,
                            name,
                            attr->attr_protocol == SYS_SYNC_PRIORITY ? "priority"
                                                                     : "fifo");
    return CELL_OK;
}

int32_t sys_event_queue_destroy(sys_event_queue_t equeue_id, int32_t mode) {
    // TODO: wakeup threads and make them return ECANCELED
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
    auto info = queues.get(equeue_id);
    INFO(libs) << ssnprintf("sys_event_queue_receive(%s[%x])", info->name, equeue_id);
    auto event = info->queue->receive(th->priority());
    th->setGPR(4, event.source);
    th->setGPR(5, event.data1);
    th->setGPR(6, event.data2);
    th->setGPR(7, event.data3);
    INFO(libs) << ssnprintf("completed sys_event_queue_receive(%s[%x]): %x, %x, %x, %x",
                            info->name,
                            equeue_id,
                            event.source,
                            event.data1,
                            event.data2,
                            event.data3);
    return CELL_OK;
}

int32_t sys_event_queue_tryreceive(sys_event_queue_t equeue_id,
                                   uint32_t event_array,
                                   int32_t size,
                                   big_uint32_t *number,
                                   PPUThread* th)
{
    auto info = queues.get(equeue_id);
    INFO(libs) << ssnprintf("sys_event_queue_tryreceive(%s[%x])", info->name, equeue_id);
    std::vector<sys_event_t> vec(size);
    size_t num;
    info->queue->tryReceive(&vec[0], vec.size(), &num);
    *number = num;
    g_state.mm->writeMemory(event_array, &vec[0], sizeof(sys_event_t) * *number);
    INFO(libs) << ssnprintf("completed sys_event_queue_tryreceive(%s[%x])", info->name, equeue_id);
    return CELL_OK;
}

int32_t sys_event_port_create(sys_event_port_t* eport_id, int32_t port_type, uint64_t name) {
    auto port = std::make_shared<queue_port_t>();
    port->name = name;
    port->type = port_type;
    *eport_id = ports.create(port);
    INFO(libs) << ssnprintf("sys_event_port_create(id = %d, name = %llx)", *eport_id, name);
    return CELL_OK;
}

int32_t sys_event_port_connect_local(sys_event_port_t event_port_id, 
                                     sys_event_queue_t event_queue_id)
{
    INFO(libs) << ssnprintf("sys_event_port_connect_local(port = %d, queue = %x)",
                            event_port_id, event_queue_id);
    assert(ports.get(event_port_id)->type == SYS_EVENT_PORT_LOCAL);
    ports.get(event_port_id)->queue = queues.get(event_queue_id)->queue;
    return CELL_OK;
}

int32_t sys_event_port_send(sys_event_port_t eport_id,
                            uint64_t data1,
                            uint64_t data2,
                            uint64_t data3) {
    INFO(libs) << ssnprintf("sys_event_port_send(port = %d, data = %llx, %llx, %llx)",
                            eport_id, data1, data2, data3);
    auto port = ports.get(eport_id);
    port->queue->send({ port->name, data1, data2, data3 });
    return CELL_OK;
}

int32_t sys_event_queue_drain(sys_event_queue_t equeue_id) {
    queues.get(equeue_id)->queue->drain();
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
                                     uint8_t spup) {
    auto info = queues.get(eq);
    INFO(libs) << ssnprintf(
        "sys_spu_thread_connect_event(thread=%x, queue=%s|%x, spup=%02x)",
        thread_id,
        info->name,
        eq,
        spup);
    assert(et == SYS_SPU_THREAD_EVENT_USER);
    auto thread = g_state.proc->getSpuThread(thread_id);
    thread->connectQueue(info->queue, spup);
    return CELL_OK;
}

int32_t sys_spu_thread_disconnect_event(uint32_t thread_id,
                                        sys_event_type_t et,
                                        uint8_t spup) {
    assert(et == SYS_SPU_THREAD_EVENT_USER);
    INFO(libs) << ssnprintf("sys_spu_thread_disconnect_event(thread = %d, spup = %02x)",
                            thread_id,
                            spup);
    auto thread = g_state.proc->getSpuThread(thread_id);
    thread->disconnectQueue(spup);
    return CELL_OK;
}

int32_t sys_spu_thread_unbind_queue(uint32_t thread_id, uint32_t spuq_num) {
    INFO(libs) << ssnprintf("sys_spu_thread_unbind_queue(thread = %d, spup = %02x)",
                            thread_id,
                            spuq_num);
    auto thread = g_state.proc->getSpuThread(thread_id);
    thread->unbindQueue(spuq_num);
    return CELL_OK;
}

int32_t sys_spu_thread_bind_queue(uint32_t thread_id,
                                  sys_event_queue_t spuq,
                                  uint32_t spuq_num) {
    auto info = queues.get(spuq);
    INFO(libs) << ssnprintf(
        "sys_spu_thread_bind_queue(thread=%d, queue=%s|%x, spuq=%02x)",
        thread_id,
        info->name,
        spuq,
        spuq_num);
    auto thread = g_state.proc->getSpuThread(thread_id);
    thread->bindQueue(info->queue, spuq_num);
    return CELL_OK;
}

int32_t sys_spu_thread_group_connect_event_all_threads(uint32_t group_id,
                                                       sys_event_queue_t eq,
                                                       uint64_t req,
                                                       uint8_t* spup) {
    auto info = queues.get(eq);
    INFO(libs) << ssnprintf("sys_spu_thread_group_connect_event_all_threads"
                            "(group=%d, queue=%s[%x], req=%016llx)", 
                            group_id, info->name, eq, req);
    auto group = findThreadGroup(group_id);
    std::vector<std::shared_ptr<SPUThread>> threads;
    std::transform(begin(group->threads), end(group->threads), std::back_inserter(threads), [=](auto id) {
        return g_state.proc->getSpuThread(id);
    });
    for (auto i = 0u; i < 64; ++i) {
        if (((1ull << i) & req) == 0)
            continue;
        auto available = std::all_of(begin(threads), end(threads), [=](auto& th) {
            return th->isQueuePortAvailableToConnect(i);
        });
        if (available) {
            for (auto& th : threads) {
                th->connectQueue(info->queue, i);
            }
            *spup = i;
            INFO(libs) << ssnprintf("connected to spup %d", i);
            return CELL_OK;
        }
    }
    assert(false);
    return -1;
}
