#pragma once

#include "../sys_defs.h"
#include <array>

#define CELL_SPURS_MAX_TASK 128
#define CELL_SPURS_ATTRIBUTE_SIZE 512
#define CELL_SPURS_JOBCHAIN_ATTRIBUTE_SIZE 512
#define CELL_SPURS_SIZE 4096
#define CELL_SPURS_SIZE2 8192
#define CELL_SPURS_JOBCHAIN_SIZE 272
#define CELL_SPURS_TASKSET_ATTRIBUTE_SIZE 512
#define CELL_SPURS_TASKSET_CLASS0_SIZE (128 * 50)
#define CELL_SPURS_TASKSET_SIZE CELL_SPURS_TASKSET_CLASS0_SIZE
#define _CELL_SPURS_TASKSET_CLASS1_EXTENDED_SIZE (128 + 128 * 16 + 128 * 15)
#define CELL_SPURS_TASKSET_CLASS1_SIZE \
    (CELL_SPURS_TASKSET_CLASS0_SIZE + _CELL_SPURS_TASKSET_CLASS1_EXTENDED_SIZE)
#define CELL_SPURS_TASKSET2_SIZE (CELL_SPURS_TASKSET_CLASS1_SIZE)
#define CELL_SPURS_TASKSET_ATTRIBUTE2_SIZE CELL_SPURS_TASKSET_ATTRIBUTE_SIZE
#define CELL_SPURS_MAX_TASK_NAME_LENGTH 32

using spurs_name_t = std::array<char, 15>;
using spurs_priority_table_t = std::array<uint8_t, 8>;
using CellSpursTaskId = big_uint32_t;

class Process;
class MainMemory;

struct CellSpursAttribute {
    char prefix[15];
    int32_t spuThreadGroupType;
    uint32_t revision;
    uint32_t sdkVersion;
    uint32_t nSpus;
    int32_t spuPriority;
    int32_t ppuPriority;
    bool exitIfNoWork;
    bool enableSpuPrintf;
};

struct CellSpursJobChainAttribute {
    ps3_uintptr_t name;
    uint32_t jmRevision;
    uint32_t sdkRevision;
    ps3_uintptr_t jobChainEntry;
    uint16_t sizeJobDescriptor;
    uint16_t maxGrabdedJob;
    spurs_priority_table_t priorityTable;
    uint32_t maxContention;
    bool autoRequestSpuCount;
    uint32_t tag1;
    uint32_t tag2;
    bool isFixedMemAlloc;
    uint32_t maxSizeJobDescriptor;
    uint32_t initialRequestSpuCount;
};

struct CellSpurs {
    
};

struct CellSpurs2 : public CellSpurs {
    
};

struct CellSpursJobChain {
    
};

struct CellSpursTasksetAttribute {
    
};

struct CellSpursTaskset {
    
};

struct CellSpursTaskset2 : public CellSpursTaskset {
    
};

struct CellSpursTaskNameBuffer {
    
};

struct CellSpursTasksetAttribute2 {
    big_uint32_t revision;
    big_uint32_t name;
    big_uint64_t argTaskset;
    big_uint8_t priority[8];
    big_uint32_t maxContention;
    big_int32_t enableClearLs;
    big_uint32_t taskNameBuffer; // CellSpursTaskNameBuffer
};

union CellSpursTaskLsPattern {
    big_uint32_t u32[4];
    big_uint64_t u64[2];
};

union CellSpursTaskArgument {
    big_uint32_t u32[4];
    big_uint64_t u64[2];
};

struct CellSpursTaskBinInfo {
    big_uint64_t eaElf;
    big_uint32_t sizeContext;
    big_uint32_t __reserved__;
    CellSpursTaskLsPattern lsPattern;
};

static_assert(sizeof(CellSpursTaskNameBuffer) <= CELL_SPURS_MAX_TASK * CELL_SPURS_MAX_TASK_NAME_LENGTH, "");
static_assert(sizeof(CellSpursTaskset2) <= CELL_SPURS_TASKSET2_SIZE - CELL_SPURS_TASKSET_SIZE, "");
static_assert(sizeof(CellSpursTaskset) <= CELL_SPURS_TASKSET_SIZE, "");
static_assert(sizeof(CellSpursTasksetAttribute) <= CELL_SPURS_TASKSET_ATTRIBUTE_SIZE, "");
static_assert(sizeof(CellSpursAttribute) <= CELL_SPURS_ATTRIBUTE_SIZE, "");
static_assert(sizeof(CellSpursJobChainAttribute) <= CELL_SPURS_JOBCHAIN_ATTRIBUTE_SIZE, "");
static_assert(sizeof(CellSpurs2) <= CELL_SPURS_SIZE2, "");
static_assert(sizeof(CellSpursJobChain) <= CELL_SPURS_JOBCHAIN_SIZE, "");
static_assert(sizeof(CellSpursTasksetAttribute2) <= CELL_SPURS_TASKSET_ATTRIBUTE2_SIZE, "");

int32_t cellSpursAttributeSetNamePrefix(CellSpursAttribute* attr,
                                        spurs_name_t* name,
                                        uint32_t size);
int32_t cellSpursAttributeEnableSpuPrintfIfAvailable(CellSpursAttribute* attr);
int32_t cellSpursAttributeSetSpuThreadGroupType(CellSpursAttribute* attr, int32_t type);
int32_t cellSpursInitializeWithAttribute(CellSpurs*, const CellSpursAttribute*, Process* proc);
int32_t cellSpursInitializeWithAttribute2(CellSpurs2*, const CellSpursAttribute*, Process* proc);
int32_t _cellSpursAttributeInitialize(CellSpursAttribute* attr,
                                      uint32_t revision,
                                      uint32_t sdkVersion,
                                      uint32_t nSpus,
                                      int32_t spuPriority,
                                      int32_t ppuPriority,
                                      bool exitIfNoWork);
int32_t cellSpursFinalize(CellSpurs2* spurs);

int32_t cellSpursJobChainAttributeSetName(CellSpursJobChainAttribute* attr, 
                                          ps3_uintptr_t name);
int32_t cellSpursCreateJobChainWithAttribute(CellSpurs2 *spurs,
                                             CellSpursJobChain *jobChain,
                                             const CellSpursJobChainAttribute *attr);
int32_t _cellSpursJobChainAttributeInitialize(uint32_t jmRevision,
                                              uint32_t sdkRevision,
                                              CellSpursJobChainAttribute *attr,
                                              ps3_uintptr_t jobChainEntry,
                                              uint16_t sizeJobDescriptor,
                                              uint16_t maxGrabdedJob,
                                              const spurs_priority_table_t* priorityTable,
                                              uint32_t maxContention,
                                              bool autoRequestSpuCount,
                                              uint32_t tag1,
                                              uint32_t tag2,
                                              bool isFixedMemAlloc,
                                              uint32_t maxSizeJobDescriptor,
                                              uint32_t initialRequestSpuCount);
int32_t cellSpursJoinJobChain(CellSpursJobChain *jobChain);
int32_t cellSpursRunJobChain(const CellSpursJobChain* jobChain);
int32_t _cellSpursTasksetAttributeInitialize(CellSpursTasksetAttribute* pAttr,
                                             uint32_t revision,
                                             uint32_t sdkVersion,
                                             uint64_t argTaskset,
                                             ps3_uintptr_t priority,
                                             uint32_t max_contention);
int32_t cellSpursTasksetAttributeSetName(CellSpursTasksetAttribute* attr, cstring_ptr_t name);
int32_t cellSpursCreateTasksetWithAttribute(CellSpurs* spurs,
                                            CellSpursTaskset* taskset,
                                            const CellSpursTasksetAttribute* attribute);
int32_t cellSpursDestroyTaskset2(CellSpursTaskset2 *pTaskset);
int32_t cellSpursCreateTaskset2(CellSpurs* pSpurs,
                                CellSpursTaskset2* pTaskset,
                                const CellSpursTasksetAttribute2* pAttr);
int32_t cellSpursJoinTask2(CellSpursTaskset2* pTaskset,
                           CellSpursTaskId idTask,
                           int32_t* exitCode);
emu_void_t _cellSpursTasksetAttribute2Initialize(CellSpursTasksetAttribute2* pAttr,
                                                 uint32_t revision);
int32_t cellSpursCreateTask2WithBinInfo(CellSpursTaskset2* taskset,
                                        CellSpursTaskId* id,
                                        const CellSpursTaskBinInfo* binInfo,
                                        const CellSpursTaskArgument* argument,
                                        ps3_uintptr_t contextBuffer,
                                        cstring_ptr_t name,
                                        uint64_t __reserved__,
                                        Process* proc);

typedef enum CellSpursEventFlagClearMode {
    CELL_SPURS_EVENT_FLAG_CLEAR_AUTO = 0,
    CELL_SPURS_EVENT_FLAG_CLEAR_MANUAL = 1
} CellSpursEventFlagClearMode;

typedef enum CellSpursEventFlagDirection {
    CELL_SPURS_EVENT_FLAG_SPU2SPU,
    CELL_SPURS_EVENT_FLAG_SPU2PPU,
    CELL_SPURS_EVENT_FLAG_PPU2SPU,
    CELL_SPURS_EVENT_FLAG_ANY2ANY
} CellSpursEventFlagDirection;

int32_t _cellSpursEventFlagInitialize(CellSpurs* spurs,
                                      CellSpursTaskset* taskset,
                                      uint32_t eventFlag,
                                      CellSpursEventFlagClearMode clearMode,
                                      CellSpursEventFlagDirection direction,
                                      MainMemory* mm);
