#pragma once

#include "../ps3emu/Process.h"
#include "sys_defs.h"

class SpuImage;
using sys_spu_thread_t = big_uint32_t;
using sys_spu_thread_group_t = big_uint32_t;
using sys_memory_container_t = big_uint32_t;

struct sys_spu_segment_t {
    big_int32_t type;
    big_uint32_t ls_start;
    big_int32_t size;
    union {
        big_uint32_t pa_start;
        big_uint32_t value;
        big_uint64_t pad;
    } src;
};

union sys_spu_image_t {
    struct {
        big_uint32_t type;
        big_uint32_t entry_point;
        big_uint32_t segs;
        big_int32_t nsegs;
    } s;
    SpuImage* elf;
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

int32_t sys_spu_initialize(uint32_t max_usable_spu, uint32_t max_raw_spu);
int32_t sys_spu_thread_read_ls(sys_spu_thread_t id,
                               uint32_t address,
                               big_uint64_t* value,
                               size_t type);
int32_t sys_spu_image_import(sys_spu_image_t* img,
                             ps3_uintptr_t src,
                             uint32_t type,
                             MainMemory* mm);
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