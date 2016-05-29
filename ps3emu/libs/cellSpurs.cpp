#include "cellSpurs.h"

#include "../utils.h"
#include <boost/log/trivial.hpp>

int32_t cellSpursAttributeSetNamePrefix(CellSpursAttribute* attr,
                                        spurs_name_t* name,
                                        uint32_t size)
{
    BOOST_LOG_TRIVIAL(trace) << ssnprintf("cellSpursAttributeSetNamePrefix(\"%s\")", name);
    strncpy(attr->prefix, name->data(), std::min<uint32_t>(size, 15));
    return CELL_OK;
}

int32_t cellSpursAttributeEnableSpuPrintfIfAvailable(CellSpursAttribute* attr) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursAttributeSetSpuThreadGroupType(CellSpursAttribute* attr, int32_t type) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    attr->spuThreadGroupType = type;
    return CELL_OK;
}

int32_t cellSpursJobChainAttributeSetName(CellSpursJobChainAttribute* attr,
                                          ps3_uintptr_t name)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
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
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    attr->revision = revision;
    attr->sdkVersion = sdkVersion;
    attr->nSpus = nSpus;
    attr->spuPriority = spuPriority;
    attr->ppuPriority = ppuPriority;
    attr->exitIfNoWork = exitIfNoWork;
    return CELL_OK;
}

int32_t cellSpursFinalize(CellSpurs2* spurs) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursInitializeWithAttribute2(CellSpurs2*, const CellSpursAttribute* attr) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursInitializeWithAttribute(CellSpurs*, const CellSpursAttribute*) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

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
                                              uint32_t initialRequestSpuCount)
{
    attr->jmRevision = jmRevision;
    attr->sdkRevision = sdkRevision;
    attr->jobChainEntry = jobChainEntry;
    attr->sizeJobDescriptor = sizeJobDescriptor;
    attr->maxGrabdedJob = maxGrabdedJob;
    attr->priorityTable = *priorityTable;
    attr->maxContention = maxContention;
    attr->autoRequestSpuCount = autoRequestSpuCount;
    attr->tag1 = tag1;
    attr->tag2 = tag2;
    attr->isFixedMemAlloc = isFixedMemAlloc;
    attr->maxSizeJobDescriptor = maxSizeJobDescriptor;
    attr->initialRequestSpuCount = initialRequestSpuCount;
    return CELL_OK;
}

int32_t cellSpursCreateJobChainWithAttribute(CellSpurs2 *spurs,
                                             CellSpursJobChain *jobChain,
                                             const CellSpursJobChainAttribute *attr)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursJoinJobChain(CellSpursJobChain *jobChain) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;    
}

int32_t cellSpursRunJobChain(const CellSpursJobChain* jobChain) {
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__;
    return CELL_OK;
}

int32_t _cellSpursTasksetAttributeInitialize(CellSpursTasksetAttribute* pAttr,
                                             uint32_t revision,
                                             uint32_t sdkVersion,
                                             uint64_t argTaskset,
                                             ps3_uintptr_t priority,
                                             uint32_t max_contention) {
    return CELL_OK;
}

int32_t cellSpursTasksetAttributeSetName(CellSpursTasksetAttribute* attr, cstring_ptr_t name) {
    return CELL_OK;
}

int32_t cellSpursCreateTasksetWithAttribute(CellSpurs* spurs,
                                            CellSpursTaskset* taskset,
                                            const CellSpursTasksetAttribute* attribute) {
    return CELL_OK;
}