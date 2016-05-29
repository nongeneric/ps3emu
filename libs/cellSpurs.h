#pragma once

#include "sys_defs.h"
#include <array>

#define CELL_SPURS_ATTRIBUTE_SIZE 512
#define CELL_SPURS_JOBCHAIN_ATTRIBUTE_SIZE 512
#define CELL_SPURS_SIZE 4096
#define CELL_SPURS_SIZE2 8192
#define CELL_SPURS_JOBCHAIN_SIZE 272

using spurs_name_t = std::array<char, 15>;
using spurs_priority_table_t = std::array<uint8_t, 8>;

struct CellSpursAttribute {
    char prefix[15];
    int32_t spuThreadGroupType;
    uint32_t revision;
    uint32_t sdkVersion;
    uint32_t nSpus;
    int32_t spuPriority;
    int32_t ppuPriority;
    bool exitIfNoWork;
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

struct CellSpurs2 {
    
};

struct CellSpursJobChain {
    
};

#define CELL_SPURS_TASKSET_ATTRIBUTE_ALIGN 8
#define CELL_SPURS_TASKSET_ATTRIBUTE_SIZE 512

typedef struct CellSpursTasksetAttribute {
    unsigned char skip[CELL_SPURS_TASKSET_ATTRIBUTE_SIZE];
} CellSpursTasksetAttribute;

#define CELL_SPURS_TASKSET_CLASS0_SIZE            (128 * 50)
#define CELL_SPURS_TASKSET_SIZE CELL_SPURS_TASKSET_CLASS0_SIZE

typedef struct CellSpursTaskset {
        unsigned char skip[CELL_SPURS_TASKSET_SIZE];
} CellSpursTaskset;

static_assert(sizeof(CellSpursAttribute) <= CELL_SPURS_ATTRIBUTE_SIZE, "");
static_assert(sizeof(CellSpursJobChainAttribute) <= CELL_SPURS_JOBCHAIN_ATTRIBUTE_SIZE, "");
static_assert(sizeof(CellSpurs2) <= CELL_SPURS_SIZE2, "");
static_assert(sizeof(CellSpursJobChain) <= CELL_SPURS_JOBCHAIN_SIZE, "");

int32_t cellSpursAttributeSetNamePrefix(CellSpursAttribute* attr,
                                        spurs_name_t* name,
                                        uint32_t size);
int32_t cellSpursAttributeEnableSpuPrintfIfAvailable(CellSpursAttribute* attr);
int32_t cellSpursAttributeSetSpuThreadGroupType(CellSpursAttribute* attr, int32_t type);
int32_t cellSpursInitializeWithAttribute(CellSpurs*, const CellSpursAttribute*);
int32_t cellSpursInitializeWithAttribute2(CellSpurs2*, const CellSpursAttribute*);
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