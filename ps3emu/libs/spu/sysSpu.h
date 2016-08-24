#pragma once

#include "ps3emu/Process.h"
#include "../sys_defs.h"

using sys_spu_thread_t = big_uint32_t;
using sys_spu_thread_group_t = big_uint32_t;
using sys_memory_container_t = big_uint32_t;
using sys_raw_spu_t = big_uint32_t;
using sys_interrupt_tag_t = big_uint32_t;
using sys_interrupt_thread_handle_t = big_uint32_t;

struct sys_spu_segment_t {
    big_int32_t type;
    big_uint32_t ls_start;
    big_int32_t size;
    big_uint32_t pad1;
    union {
        big_uint32_t pa_start;
        big_uint32_t value;
    } src;
    big_uint32_t pad2;
};

static_assert(sizeof(sys_spu_segment_t) == 24, "");

struct sys_spu_image_t {
    big_uint32_t type;
    big_uint32_t entry_point;
    big_uint32_t segs; // sys_spu_segment_t*
    big_int32_t nsegs;
};

struct sys_spu_thread_group_attribute_t {
    big_uint32_t nsize;
    big_uint32_t name;
    big_int32_t type;
    union {
        sys_memory_container_t ct;
    } option;
};

struct sys_spu_thread_attribute_t {
    big_uint32_t name;
    big_uint32_t nsize;
    big_uint32_t option;
};

struct sys_spu_thread_argument_t {
    big_uint64_t arg1;
    big_uint64_t arg2;
    big_uint64_t arg3;
    big_uint64_t arg4;
};

struct ThreadGroup {
    std::vector<uint32_t> threads;
    std::string name;
    std::map<uint32_t, int32_t> errorCodes;
};

int32_t sys_spu_initialize(uint32_t max_usable_spu, uint32_t max_raw_spu);
int32_t sys_spu_thread_read_ls(sys_spu_thread_t id,
                               uint32_t address,
                               uint64_t value,
                               size_t type);
int32_t sys_spu_image_import(sys_spu_image_t* img,
                             ps3_uintptr_t src,
                             uint32_t type,
                             PPUThread* proc);
int32_t sys_spu_image_open(sys_spu_image_t* img, cstring_ptr_t path, Process* proc);
int32_t sys_spu_image_close(sys_spu_image_t* img);
int32_t sys_spu_thread_group_create(sys_spu_thread_group_t* id,
                                    uint32_t num,
                                    int32_t prio,
                                    sys_spu_thread_group_attribute_t* attr,
                                    MainMemory* mm);
int32_t sys_spu_thread_initialize(sys_spu_thread_t* thread,
                                  sys_spu_thread_group_t group,
                                  uint32_t spu_num,
                                  sys_spu_image_t* img,
                                  const sys_spu_thread_attribute_t* attr,
                                  const sys_spu_thread_argument_t* arg,
                                  Process* mm);
int32_t sys_spu_thread_group_start(sys_spu_thread_group_t id, Process* proc);
int32_t sys_spu_thread_group_join(sys_spu_thread_group_t gid,
                                  big_int32_t* cause,
                                  big_int32_t* status,
                                  Process* proc);
int32_t sys_spu_thread_group_destroy(sys_spu_thread_group_t id, Process* proc);
int32_t sys_spu_thread_get_exit_status(sys_spu_thread_t id, big_int32_t* status, Process* proc);
int32_t sys_raw_spu_create(sys_raw_spu_t* id, uint32_t unused, Process* proc);
int32_t sys_raw_spu_destroy(sys_raw_spu_t id, Process* proc);
int32_t sys_raw_spu_image_load(sys_raw_spu_t id, sys_spu_image_t* img, PPUThread* th);
int32_t sys_raw_spu_load(sys_raw_spu_t id, cstring_ptr_t path, big_uint32_t* entry, Process* proc);
int32_t sys_raw_spu_create_interrupt_tag(sys_raw_spu_t id,
                                         unsigned class_id,
                                         uint32_t unused,
                                         sys_interrupt_tag_t* intrtag);
int32_t sys_interrupt_thread_establish(sys_interrupt_thread_handle_t* ih,
                                       sys_interrupt_tag_t intrtag,
                                       uint32_t intrthread,
                                       uint64_t arg,
                                       Process* proc);
int32_t sys_raw_spu_set_int_mask(sys_raw_spu_t id, uint32_t class_id, uint64_t mask);
int32_t sys_raw_spu_mmio_write(sys_raw_spu_t id,
                               unsigned classId,
                               uint32_t value);
uint32_t sys_raw_spu_mmio_read(sys_raw_spu_t id, unsigned classId);
int32_t sys_raw_spu_get_int_stat(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 big_uint64_t* stat);
int32_t sys_raw_spu_read_puint_mb(sys_raw_spu_t id, big_uint32_t* value);
int32_t sys_spu_thread_write_spu_mb(uint32_t thread_id, uint32_t value);
int32_t sys_raw_spu_set_int_stat(sys_raw_spu_t id, uint32_t class_id, uint64_t stat);
emu_void_t sys_interrupt_thread_eoi();
int32_t sys_spu_thread_group_connect_event(sys_spu_thread_group_t id,
                                           sys_event_queue_t eq,
                                           sys_event_type_t et);
int32_t sys_spu_thread_group_disconnect_event(sys_spu_thread_group_t id,
                                              sys_event_queue_t eq,
                                              sys_event_type_t et);
int32_t sys_spu_thread_group_disconnect_event_all_threads(sys_spu_thread_group_t id, uint8_t spup);
int32_t sys_spu_image_get_info(const sys_spu_image_t *img, big_uint32_t* entry_point, big_uint32_t* nsegs);
int32_t sys_spu_image_get_modules(const sys_spu_image_t *img, ps3_uintptr_t buf, uint32_t nsegs);

std::shared_ptr<SPUThread> findRawSpuThread(sys_raw_spu_t id);
std::shared_ptr<ThreadGroup> findThreadGroup(sys_spu_thread_group_t id);

class MainMemory;
class InternalMemoryManager;

void spuImageMap(MainMemory* mm, sys_spu_image_t* image, void* ls);
