#include "queue.h"

#include "../ConcurrentQueue.h"
#include "../../ps3emu/IDMap.h"
#include "../../ps3emu/MainMemory.h"
#include <boost/thread.hpp>
#include <boost/log/trivial.hpp>

namespace {    
    class IQueue {
    public:
        virtual ~IQueue() = default;
        virtual void send(sys_event_t const& t) = 0;
        virtual sys_event_t receive(unsigned priority) = 0;
        virtual void tryReceive(sys_event_t* arr, size_t size, size_t* number) = 0;
        virtual void drain() = 0;
    };
    
    template <template<class> class Q>
    class EventQueue : public IQueue, public Q<sys_event_t> {
    public:
        void send(sys_event_t const& t) override {
            Q<sys_event_t>::send(t);
        }
        sys_event_t receive(unsigned priority) override {
            return Q<sys_event_t>::receive(priority);
        }
        void tryReceive(sys_event_t* arr, size_t size, size_t* number) override {
            Q<sys_event_t>::tryReceive(arr, size, number);
        }
        void drain() override {
            Q<sys_event_t>::drain();
        }
    };
    
    struct queue_port_t {
        uint64_t name;
        int32_t type;
        IQueue* queue = nullptr;
    };

    ThreadSafeIDMap<sys_event_queue_t, std::shared_ptr<IQueue>> queues;
    ThreadSafeIDMap<sys_event_port_t, std::shared_ptr<queue_port_t>> ports;
}

#define   SYS_SYNC_FIFO                     0x00001
#define   SYS_SYNC_PRIORITY                 0x00002
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL   0x01

int32_t sys_event_queue_create(sys_event_queue_t* equeue_id,
                               sys_event_queue_attribute_t* attr,
                               sys_ipc_key_t event_queue_key,
                               uint32_t size) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(1 <= size && size < 128);
    assert(event_queue_key == SYS_EVENT_QUEUE_LOCAL);
    assert(attr->attr_protocol == SYS_SYNC_PRIORITY ||
           attr->attr_protocol == SYS_SYNC_FIFO);
    std::shared_ptr<IQueue> queue;
    if (attr->attr_protocol == SYS_SYNC_PRIORITY) {
        queue.reset(new EventQueue<ConcurrentPriorityQueue>);
    } else {
        queue.reset(new EventQueue<ConcurrentFifoQueue>);
    }
    *equeue_id = queues.create(std::move(queue));
    return CELL_OK;
}

int32_t sys_event_queue_destroy(sys_event_queue_t equeue_id, int32_t mode) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(!mode);
    queues.destroy(equeue_id);
    return CELL_OK;
}

int32_t sys_event_queue_receive(sys_event_queue_t equeue_id,
                                uint32_t unused,
                                usecond_t timeout,
                                PPUThread* th)
{
    assert(timeout == 0);
    auto queue = queues.get(equeue_id);
    auto event = queue->receive(th->priority());
    th->setGPR(4, event.source);
    th->setGPR(5, event.data1);
    th->setGPR(6, event.data2);
    th->setGPR(7, event.data3);
    return CELL_OK;
}

int32_t sys_event_queue_tryreceive(sys_event_queue_t equeue_id,
                                   uint32_t event_array,
                                   int32_t size,
                                   big_uint32_t *number,
                                   PPUThread* th)
{
    auto queue = queues.get(equeue_id);
    std::vector<sys_event_t> vec(size);
    size_t num;
    queue->tryReceive(&vec[0], vec.size(), &num);
    *number = num;
    th->mm()->writeMemory(event_array, &vec[0], sizeof(sys_event_t) * *number);
    return CELL_OK;
}

int32_t sys_event_port_create(sys_event_port_t* eport_id, int32_t port_type, uint64_t name) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    auto port = std::make_shared<queue_port_t>();
    port->name = name;
    port->type = port_type;
    *eport_id = ports.create(port);
    return CELL_OK;
}

int32_t sys_event_port_connect_local(sys_event_port_t event_port_id, 
                                 sys_event_queue_t event_queue_id)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    assert(ports.get(event_port_id)->type == SYS_EVENT_PORT_LOCAL);
    ports.get(event_port_id)->queue = queues.get(event_queue_id).get();
    return CELL_OK;
}

int32_t sys_event_port_send(sys_event_port_t eport_id,
                            uint64_t data1,
                            uint64_t data2,
                            uint64_t data3)
{
    auto port = ports.get(eport_id);
    port->queue->send({ port->name, data1, data2, data3 });
    return CELL_OK;
}

int32_t sys_event_queue_drain(sys_event_queue_t equeue_id) {
    queues.get(equeue_id)->drain();
    return CELL_OK;
}
