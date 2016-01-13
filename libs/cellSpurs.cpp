#include "cellSpurs.h"

int32_t cellSpursAttributeSetNamePrefix(CellSpursAttribute* attr,
                                        const char* name,
                                        uint32_t size)
{
    strncpy(attr->prefix, name, std::min<uint32_t>(size, 15));
    return CELL_OK;
}

int32_t cellSpursAttributeEnableSpuPrintfIfAvailable(CellSpursAttribute* attr) {
    return CELL_OK;
}

int32_t cellSpursAttributeSetSpuThreadGroupType(CellSpursAttribute* attr, int32_t type) {
    attr->spuThreadGroupType = type;
    return CELL_OK;
}

int32_t cellSpursJobChainAttributeSetName(CellSpursJobChainAttribute* attr,
                                          ps3_uintptr_t name)
{
    attr->name = name;
    return CELL_OK;
}

int32_t _cellSpursAttributeInitialize(CellSpursAttribute* attr,
                                      uint32_t revision,
                                      uint32_t sdkVersion,
                                      uint32_t nSpus,
                                      int32_t spuPriority,
                                      int32_t ppuPriority,
                                      bool exitIfNoWork)
{
    attr->revision = revision;
    attr->sdkVersion = sdkVersion;
    attr->nSpus = nSpus;
    attr->spuPriority = spuPriority;
    attr->ppuPriority = ppuPriority;
    attr->exitIfNoWork = exitIfNoWork;
    return CELL_OK;
}

int32_t cellSpursInitializeWithAttribute2(CellSpurs2*, const CellSpursAttribute* attr) {
    return CELL_OK;
}

int32_t _cellSpursJobChainAttributeInitialize(uint32_t jmRevision,
                                              uint32_t sdkRevision,
                                              CellSpursJobChainAttribute *attr,
                                              ps3_uintptr_t jobChainEntry,
                                              uint16_t sizeJobDescriptor,
                                              uint16_t maxGrabdedJob,
                                              const uint8_t priorityTable[8],
                                              uint32_t maxContention,
                                              bool autoRequestSpuCount,
                                              uint32_t tag1,
                                              uint32_t tag2,
                                              bool isFixedMemAlloc,
                                              uint32_t maxSizeJobDescriptor,
                                              uint32_t initialRequestSpuCount)
{
    attr->jmRevision = jmRevision;
    attr->sdkRevision = sdkRevision;
    attr->jobChainEntry = jobChainEntry;
    attr->sizeJobDescriptor = sizeJobDescriptor;
    attr->maxGrabdedJob = maxGrabdedJob;
    memcpy(attr->priorityTable, priorityTable, 8);
    attr->maxContention = maxContention;
    attr->autoRequestSpuCount = autoRequestSpuCount;
    attr->tag1 = tag1;
    attr->tag2 = tag2;
    attr->isFixedMemAlloc = isFixedMemAlloc;
    attr->maxSizeJobDescriptor = maxSizeJobDescriptor;
    attr->initialRequestSpuCount = initialRequestSpuCount;
    return CELL_OK;
}