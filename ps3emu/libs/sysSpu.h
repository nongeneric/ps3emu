#pragma once

#include "../Process.h"
#include "sys_defs.h"

class SpuImage;
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

#define X(k, v) k = v,
#define TagClassIdX \
    X(_MFC_LSA, 0x3004U) \
    X(_MFC_EAH, 0x3008U) \
    X(_MFC_EAL, 0x300CU) \
    X(MFC_Size_Tag, 0x3010U) \
    X(MFC_Class_CMD, 0x3014U) \
    X(MFC_QStatus, 0x3104U) \
    X(Prxy_QueryType, 0x3204U) \
    X(Prxy_QueryMask, 0x321CU) \
    X(Prxy_TagStatus, 0x322CU) \
    X(SPU_Out_MBox, 0x4004U) \
    X(SPU_In_MBox, 0x400CU) \
    X(SPU_MBox_Status, 0x4014U) \
    X(SPU_RunCntl, 0x401CU) \
    X(SPU_Status, 0x4024U) \
    X(SPU_NPC, 0x4034U) \
    X(SPU_Sig_Notify_1, 0x1400CU) \
    X(SPU_Sig_Notify_2, 0x1C00CU)
    
enum class TagClassId : uint32_t {
    TagClassIdX
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
int32_t sys_raw_spu_image_load(sys_raw_spu_t id, sys_spu_image_t* img);
int32_t sys_raw_spu_load(sys_raw_spu_t id, cstring_ptr_t path, big_uint32_t* entry, Process* proc);
int32_t sys_raw_spu_create_interrupt_tag(sys_raw_spu_t id,
                                         TagClassId class_id,
                                         uint32_t unused,
                                         sys_interrupt_tag_t* intrtag);
int32_t sys_interrupt_thread_establish(sys_interrupt_thread_handle_t* ih,
                                       sys_interrupt_tag_t intrtag,
                                       uint32_t intrthread,
                                       uint64_t arg,
                                       Process* proc);
int32_t sys_raw_spu_set_int_mask(sys_raw_spu_t id, uint32_t class_id, uint64_t mask);
int32_t sys_raw_spu_mmio_write(sys_raw_spu_t id,
                               TagClassId classId,
                               uint32_t value,
                               Process* proc);
uint32_t sys_raw_spu_mmio_read(sys_raw_spu_t id, TagClassId classId, Process* proc);
int32_t sys_raw_spu_get_int_stat(sys_raw_spu_t id,
                                 uint32_t class_id,
                                 big_uint64_t* stat);
int32_t sys_raw_spu_read_puint_mb(sys_raw_spu_t id, big_uint32_t* value);
int32_t sys_raw_spu_set_int_stat(sys_raw_spu_t id, uint32_t class_id, uint64_t stat);
emu_void_t sys_interrupt_thread_eoi();

SPUThread* findRawSpuThread(sys_raw_spu_t id);
