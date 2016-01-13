#include "queue.h"

#include "../ConcurrentQueue.h"
#include "../../ps3emu/IDMap.h"
#include "../../ps3emu/MainMemory.h"
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

namespace {
    typedef ConcurrentQueue<sys_event_t> queue_t;
    IDMap<sys_event_queue_t, std::shared_ptr<queue_t>> queues;
    boost::mutex map_mutex;
    
    struct queue_port_t {
        uint64_t name;
        int type;
        queue_t* queue = nullptr;
    };

    IDMap<sys_event_port_t, queue_port_t> ports;
}

#define   SYS_SYNC_FIFO                     0x00001
#define   SYS_SYNC_PRIORITY                 0x00002
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL   0x01

int32_t sys_event_queue_create(sys_event_queue_t* equeue_id,
                           sys_event_queue_attribute_t* attr,
                           sys_ipc_key_t event_queue_key,
                           uint32_t size)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(event_queue_key == SYS_EVENT_QUEUE_LOCAL);
    assert(attr->attr_protocol == SYS_SYNC_PRIORITY ||
           attr->attr_protocol == SYS_SYNC_FIFO);
    auto order = attr->attr_protocol == SYS_SYNC_PRIORITY ?
        QueueReceivingOrder::Priority : QueueReceivingOrder::Fifo;
    auto queue = std::make_shared<queue_t>(order, size);
    
    boost::unique_lock<boost::mutex> lock(map_mutex);
    *equeue_id = queues.create(std::move(queue));
    return CELL_OK;
}

int32_t sys_event_queue_destroy(sys_event_queue_t equeue_id, int32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(!mode);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    queues.destroy(equeue_id);
    return CELL_OK;
}

int32_t sys_event_queue_receive(sys_event_queue_t equeue_id,
                                sys_event_t* event,
                                usecond_t timeout,
                                PPUThread* th)
{
    assert(timeout == 0);
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto queue = queues.get(equeue_id);
    lock.unlock();
    *event = queue->receive(th->priority());
    return CELL_OK;
}

int32_t sys_event_queue_tryreceive(sys_event_queue_t equeue_id,
                                   uint32_t event_array,
                                   int32_t size,
                                   big_uint32_t *number,
                                   PPUThread* th)
{
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto queue = queues.get(equeue_id);
    lock.unlock();
    std::vector<sys_event_t> vec(size);
    size_t num;
    queue->tryReceive(&vec[0], vec.size(), &num);
    *number = num;
    th->mm()->writeMemory(event_array, &vec[0], sizeof(sys_event_t) * *number);
    return CELL_OK;
}

int32_t sys_event_port_create(sys_event_port_t* eport_id, int port_type, uint64_t name) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    boost::unique_lock<boost::mutex> lock(map_mutex);
    *eport_id = ports.create({ name, port_type });
    return CELL_OK;
}

int32_t sys_event_port_connect_local(sys_event_port_t event_port_id, 
                                 sys_event_queue_t event_queue_id)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    boost::unique_lock<boost::mutex> lock(map_mutex);
    assert(ports.get(event_port_id).type == SYS_EVENT_PORT_LOCAL);
    ports.get(event_port_id).queue = queues.get(event_queue_id).get();
    return CELL_OK;
}

int32_t sys_event_port_send(sys_event_port_t eport_id,
                            uint64_t data1,
                            uint64_t data2,
                            uint64_t data3)
{
    boost::unique_lock<boost::mutex> lock(map_mutex);
    auto port = ports.get(eport_id);
    port.queue->send({ port.name, data1, data2, data3 });
    return CELL_OK;
}

int32_t sys_event_queue_drain(sys_event_queue_t equeue_id) {
    boost::unique_lock<boost::mutex> lock(map_mutex);
    queues.get(equeue_id)->drain();
    return CELL_OK;
}
