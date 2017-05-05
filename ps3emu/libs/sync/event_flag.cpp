#include "event_flag.h"

#include "ps3emu/log.h"
#include "ps3emu/IDMap.h"
#include "ps3emu/utils.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace {
    struct EventFlag {
        std::string name;
        uint64_t value;
        boost::mutex m;
        boost::condition_variable cv;
    };
    
    ThreadSafeIDMap<uint32_t, std::shared_ptr<EventFlag>, EventFlagIdBase> map;
}

int32_t sys_event_flag_create(big_uint32_t* id,
                              const sys_event_flag_attribute_t* attr,
                              uint64_t init) {
    auto flag = std::make_shared<EventFlag>();
    flag->name = attr->name;
    flag->value = init;
    *id = map.create(flag);
    INFO(libs) << ssnprintf("sys_event_flag_create(%x, %x, %s)", *id, init, std::string(attr->name, 8));
    return CELL_OK;
}

int32_t sys_event_flag_destroy(uint32_t id) {
    map.destroy(id);
    return CELL_OK;
}

int32_t sys_event_flag_wait(uint32_t id,
                            uint64_t bitptn,
                            uint32_t mode,
                            big_uint64_t* result,
                            usecond_t timeout) {
    assert(timeout == 0);
    auto flag = map.get(id);
    boost::unique_lock<boost::mutex> lock(flag->m);
    flag->cv.wait(lock, [&]{ 
        auto masked = flag->value & bitptn;
        return (mode & SYS_EVENT_FLAG_WAIT_AND) ? masked == bitptn : masked;
    });
    if (result) {
        *result = flag->value;
    }
    if (mode & SYS_EVENT_FLAG_WAIT_CLEAR) {
        flag->value &= ~bitptn;
    } else if (mode & SYS_EVENT_FLAG_WAIT_CLEAR_ALL) {
        flag->value = 0;
    }
    return CELL_OK;
}

int32_t sys_event_flag_set(uint32_t id, uint64_t bitptn) {
    INFO(libs) << ssnprintf("sys_event_flag_set(%x, %llx)", id, bitptn);
    auto flag = map.get(id);
    boost::unique_lock<boost::mutex> lock(flag->m);
    flag->value |= bitptn;
    flag->cv.notify_all();
    return CELL_OK;
}

int32_t sys_event_flag_get(uint32_t id, big_uint64_t* value) {
    INFO(libs) << ssnprintf("sys_event_flag_set(%x)", id);
    auto flag = map.get(id);
    boost::unique_lock<boost::mutex> lock(flag->m);
    *value = flag->value;
    return CELL_OK;
}

int32_t sys_event_flag_clear(uint32_t id, uint64_t bitptn) {
    INFO(libs) << ssnprintf("sys_event_flag_set(%x, %llx)", id, bitptn);
    auto flag = map.get(id);
    boost::unique_lock<boost::mutex> lock(flag->m);
    flag->value &= bitptn;
    return CELL_OK;
}
