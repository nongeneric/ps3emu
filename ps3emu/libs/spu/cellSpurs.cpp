#include "cellSpurs.h"

#include "SpuImage.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/Process.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/utils.h"
#include "ps3emu/log.h"

#define CELL_SPURS_EVENT_FLAG_SIZE		128

int32_t cellSpursAttributeSetNamePrefix(CellSpursAttribute* attr,
                                        spurs_name_t* name,
                                        uint32_t size)
{
    LOG << ssnprintf("cellSpursAttributeSetNamePrefix(\"%s\")", name);
    strncpy(attr->prefix, name->data(), std::min<uint32_t>(size, 15));
    return CELL_OK;
}

int32_t cellSpursAttributeEnableSpuPrintfIfAvailable(CellSpursAttribute* attr) {
    LOG << __FUNCTION__;
    attr->enableSpuPrintf = true;
    return CELL_OK;
}

int32_t cellSpursAttributeSetSpuThreadGroupType(CellSpursAttribute* attr, int32_t type) {
    LOG << __FUNCTION__;
    attr->spuThreadGroupType = type;
    return CELL_OK;
}

int32_t cellSpursJobChainAttributeSetName(CellSpursJobChainAttribute* attr,
                                          ps3_uintptr_t name)
{
    LOG << __FUNCTION__;
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
    LOG << __FUNCTION__;
    attr->revision = revision;
    attr->sdkVersion = sdkVersion;
    attr->nSpus = nSpus;
    attr->spuPriority = spuPriority;
    attr->ppuPriority = ppuPriority;
    attr->exitIfNoWork = exitIfNoWork;
    attr->enableSpuPrintf = false;
    return CELL_OK;
}

int32_t cellSpursFinalize(CellSpurs2* spurs) {
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursInitializeWithAttribute2(CellSpurs2* spurs2, const CellSpursAttribute* attr, Process* proc) {
    LOG << __FUNCTION__;
    cellSpursInitializeWithAttribute(spurs2, attr, proc);
    return CELL_OK;
}

int32_t cellSpursInitializeWithAttribute(CellSpurs* spurs, const CellSpursAttribute* attr, Process* proc) {
    LOG << __FUNCTION__;
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
    LOG << __FUNCTION__;
    return CELL_OK;
}

int32_t cellSpursJoinJobChain(CellSpursJobChain *jobChain) {
    LOG << __FUNCTION__;
    return CELL_OK;    
}

int32_t cellSpursRunJobChain(const CellSpursJobChain* jobChain) {
    LOG << __FUNCTION__;
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

int32_t cellSpursDestroyTaskset2(CellSpursTaskset2* pTaskset) {
    return CELL_OK;
}

int32_t cellSpursCreateTaskset2(CellSpurs* pSpurs,
                                CellSpursTaskset2* pTaskset,
                                const CellSpursTasksetAttribute2* pAttr) {
    return CELL_OK;                                    
}
                                
int32_t cellSpursJoinTask2(CellSpursTaskset2* pTaskset,
                           CellSpursTaskId idTask,
                           int32_t* exitCode) {
    return CELL_OK;                               
}

emu_void_t _cellSpursTasksetAttribute2Initialize(CellSpursTasksetAttribute2* pAttr,
                                                 uint32_t revision) {
    return CELL_OK;                                                     
}

void spuPrintf(void* ls, const uint8_t* regs, uint32_t size) {
    R128 r[10];
    for (int i = 0; i < 10; ++i) {
        r[i].load(&regs[i * sizeof(R128)]);
    }
    printf((const char*)ls + r[0].w<0>(),
           r[1].w<0>(),
           r[2].w<0>(),
           r[3].w<0>(),
           r[4].w<0>(),
           r[5].w<0>(),
           r[6].w<0>(),
           r[7].w<0>(),
           r[8].w<0>(),
           r[9].w<0>());
}

int32_t cellSpursCreateTask2WithBinInfo(CellSpursTaskset2* taskset,
                                        CellSpursTaskId* id,
                                        const CellSpursTaskBinInfo* binInfo,
                                        const CellSpursTaskArgument* argument,
                                        ps3_uintptr_t contextBuffer,
                                        cstring_ptr_t name,
                                        uint64_t __reserved__,
                                        Process* proc) {
    SpuImage image([=](uint32_t ptr, void* buf, size_t size) {
        proc->mm()->readMemory(ptr, buf, size);
    }, binInfo->eaElf);
    auto spuThreadId = proc->createSpuThread(name.str);
    auto spuThread = proc->getSpuThread(spuThreadId);
    memcpy(spuThread->ptr(0), image.localStorage(), LocalStorageSize);
    spuThread->setNip(image.entryPoint());
    spuThread->setElfSource(image.source());
    spuThread->r(3).dw<0>() = argument->u64[0];
    spuThread->r(3).dw<1>() = argument->u64[1];
    // set r4 to taskset argument
    spuThread->setInterruptHandler([&] {
        auto& imbox = spuThread->getFromSpuInterruptMailbox();
        auto& mbox = spuThread->getFromSpuMailbox();
        auto size = imbox.receive(0);
        auto regs = spuThread->ptr(mbox.receive(0));
        spuPrintf(spuThread->ptr(0), regs, size);
    });
    spuThread->run();
    return CELL_OK;
}

int32_t _cellSpursEventFlagInitialize(CellSpurs* spurs,
                                      CellSpursTaskset* taskset,
                                      uint32_t eventFlag,
                                      CellSpursEventFlagClearMode clearMode,
                                      CellSpursEventFlagDirection direction,
                                      MainMemory* mm) {
    mm->setMemory(eventFlag, 0, CELL_SPURS_EVENT_FLAG_SIZE);
    mm->store<4>(eventFlag + 3 * 4, 0xff000000);
    return CELL_OK;
}
