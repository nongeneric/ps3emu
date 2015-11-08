#include "ConcurrentQueue.h"
#include "../ps3emu/utils.h"
#include "sys.h"
#include <time.h>
#include <stdio.h>
#include <stdexcept>
#include "../ps3emu/PPU.h"
#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/log/trivial.hpp>
#include <memory>
#include <map>

void init_sys_lib() { }

void sys_initialize_tls(uint64_t undef, uint32_t unk1, uint32_t unk2) {
    
}

int sys_lwmutex_create(sys_lwmutex_t* mutex_id, sys_lwmutex_attribute_t* attr) {
    return CELL_OK;
}

int sys_lwmutex_destroy(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}

int sys_lwmutex_lock(sys_lwmutex_t* lwmutex_id, usecond_t timeout) {
    return CELL_OK;
}

int sys_lwmutex_unlock(sys_lwmutex_t* lwmutex_id) {
    return CELL_OK;
}

int sys_memory_get_user_memory_size(sys_memory_info_t* mem_info) {
    mem_info->available_user_memory = 256 * (1 << 20);
    mem_info->total_user_memory = 512 * (1 << 20);
    return CELL_OK;
}

system_time_t sys_time_get_system_time(PPU* ppu) {
    auto sec = (float)ppu->getTimeBase() / (float)ppu->getFrequency();
    return sec * 1000000;
}

int _sys_process_atexitspawn() {
    return CELL_OK;
}

int _sys_process_at_Exitspawn() {
    return CELL_OK;
}

int sys_ppu_thread_get_id(sys_ppu_thread_t* thread_id) {
    *thread_id = 7;
    return CELL_OK;
}

#define SYS_TTYP_MAX  (0x10)

#define SYS_TTYP0     (0)
#define SYS_TTYP1     (1)
#define SYS_TTYP2     (2)
#define SYS_TTYP3     (3)
#define SYS_TTYP4     (4)
#define SYS_TTYP5     (5)
#define SYS_TTYP6     (6)
#define SYS_TTYP7     (7)
#define SYS_TTYP8     (8)
#define SYS_TTYP9     (9)
#define SYS_TTYP10   (10)
#define SYS_TTYP11   (11)
#define SYS_TTYP12   (12)
#define SYS_TTYP13   (13)
#define SYS_TTYP14   (14)
#define SYS_TTYP15   (15)

#define  SYS_TTYP_PPU_STDIN    (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDOUT   (SYS_TTYP0)
#define  SYS_TTYP_PPU_STDERR   (SYS_TTYP1)
#define  SYS_TTYP_SPU_STDOUT   (SYS_TTYP2)
#define  SYS_TTYP_USER1        (SYS_TTYP3)
#define  SYS_TTYP_USER2        (SYS_TTYP4)
#define  SYS_TTYP_USER3        (SYS_TTYP5)
#define  SYS_TTYP_USER4        (SYS_TTYP6)
#define  SYS_TTYP_USER5        (SYS_TTYP7)
#define  SYS_TTYP_USER6        (SYS_TTYP8)
#define  SYS_TTYP_USER7        (SYS_TTYP9)
#define  SYS_TTYP_USER8        (SYS_TTYP10)
#define  SYS_TTYP_USER9        (SYS_TTYP11)
#define  SYS_TTYP_USER10       (SYS_TTYP12)
#define  SYS_TTYP_USER11       (SYS_TTYP13)
#define  SYS_TTYP_USER12       (SYS_TTYP14)
#define  SYS_TTYP_USER13       (SYS_TTYP15)

int sys_tty_write(unsigned int ch, const void* buf, unsigned int len, unsigned int* pwritelen) {
    if (len == 0)
        return CELL_OK;
    auto asChar = (const char*)buf;
    if (ch == SYS_TTYP_PPU_STDOUT) {
        fwrite(buf, 1, len, stdout);
        fflush(stdout);
        BOOST_LOG_TRIVIAL(trace) << "stdout: " << std::string(asChar, asChar + len);
        return CELL_OK;
    }
    if (ch == SYS_TTYP_PPU_STDERR) {
        fwrite(buf, 1, len, stderr);
        fflush(stderr);
        BOOST_LOG_TRIVIAL(trace) << "stderr: " << std::string(asChar, asChar + len);
        return CELL_OK;
    }
    throw std::runtime_error(ssnprintf("unknown channel %d", ch));
}

int sys_dbg_set_mask_to_ppu_exception_handler(uint64_t mask, uint64_t flags) {
    return CELL_OK;
}

int sys_prx_exitspawn_with_level(uint64_t level) {
    return CELL_OK;
}

int sys_memory_allocate(size_t size, uint64_t flags, sys_addr_t* alloc_addr, PPU* ppu) {
    *alloc_addr = ppu->malloc(size);
    return CELL_OK;
}

int sys_timer_usleep(usecond_t sleep_time) {
    boost::this_thread::sleep_for( boost::chrono::microseconds(sleep_time) );
    return CELL_OK;
}

// TODO: filesystem impl / tests

CellFsErrno sys_fs_open_impl(const char* path,
                             uint32_t flags,
                             big_uint32_t* fd,
                             uint64_t mode,
                             const void* arg,
                             uint64_t size)
{
    return 1;
}

boost::mutex eventQueueVectorMutex;
typedef ConcurrentQueue<sys_event_t> queue_t;
std::map<sys_event_queue_t, std::unique_ptr<queue_t>> queues;

struct queue_port_t {
    big_uint64_t name;
    int type;
    queue_t* queue = nullptr;
};

std::map<sys_event_port_t, queue_port_t> ports;

#define   SYS_SYNC_FIFO                     0x00001
#define   SYS_SYNC_PRIORITY                 0x00002
#define SYS_EVENT_QUEUE_LOCAL 0x00
#define SYS_EVENT_PORT_NO_NAME 0x00
#define SYS_EVENT_PORT_LOCAL   0x01

int sys_event_queue_create(sys_event_queue_t* equeue_id,
                           sys_event_queue_attribute_t* attr,
                           sys_ipc_key_t event_queue_key,
                           uint32_t size)
{
    assert(event_queue_key == SYS_EVENT_QUEUE_LOCAL);
    assert(attr->attr_protocol == SYS_SYNC_PRIORITY ||
           attr->attr_protocol == SYS_SYNC_FIFO);
    auto order = attr->attr_protocol == SYS_SYNC_PRIORITY ?
        QueueReceivingOrder::Priority : QueueReceivingOrder::Fifo;
    auto queue = std::make_unique<queue_t>(order, size);
    
    boost::lock_guard<boost::mutex> lock(eventQueueVectorMutex);
    auto it = std::max_element(begin(queues), end(queues), [](auto& a, auto& b) {
        return a.first < b.first;
    });
    auto maxid = it == end(queues) ? sys_event_queue_t() : it->first;
    *equeue_id = maxid + 1;
    queues[maxid + 1] = std::move(queue);
    return CELL_OK;
}

int sys_event_port_create(sys_event_port_t* eport_id, int port_type, uint64_t name) {
    boost::lock_guard<boost::mutex> lock(eventQueueVectorMutex);
    auto it = std::max_element(begin(ports), end(ports), [](auto& a, auto& b) {
        return a.first < b.first;
    });
    auto maxid = it == end(ports) ? sys_event_port_t() : it->first;
    *eport_id = maxid + 1;
    ports[maxid + 1] = { name, port_type };
    return CELL_OK;
}

int sys_event_port_connect_local(sys_event_port_t event_port_id, 
                                 sys_event_queue_t event_queue_id)
{
    boost::lock_guard<boost::mutex> lock(eventQueueVectorMutex);
    assert(ports.find(event_port_id) != end(ports));
    assert(queues.find(event_queue_id) != end(queues));
    assert(ports[event_port_id].type == SYS_EVENT_PORT_LOCAL);
    ports[event_port_id].queue = queues[event_queue_id].get();
    return CELL_OK;
}

uint64_t sys_time_get_timebase_frequency(PPU* ppu) {
    return ppu->getFrequency();
}

int32_t sys_ppu_thread_get_stack_information(sys_ppu_thread_stack_t* info) {
    info->pst_addr = StackBase;
    info->pst_size = StackSize;
    return CELL_OK;
}

