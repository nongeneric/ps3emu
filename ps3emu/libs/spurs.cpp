#include "spurs.h"

#include "ps3emu/state.h"
#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include <string>

emu_void_t _cellSpursTaskAttribute2Initialize_proxy(const CellSpursTaskAttribute2 *pAttr) {
    INFO(libs) << "cellSpursTaskAttribute2Initialize";
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursCreateTaskset2_proxy(uint32_t spurs,
                                         uint32_t taskset2,
                                         const CellSpursTasksetAttribute2* attr) {
    std::string name = "NULL";
    if (attr->nameVa) {
        readString(g_state.mm, attr->nameVa, name);
    }
    auto priority = ssnprintf("{%d,%d,%d,%d,%d,%d,%d,%d}",
                              attr->priority[0],
                              attr->priority[1],
                              attr->priority[2],
                              attr->priority[3],
                              attr->priority[4],
                              attr->priority[5],
                              attr->priority[6],
                              attr->priority[7]);
    INFO(libs) << ssnprintf("cellSpursCreateTaskset2(%x, %s, %llx, %s, %d, %d)",
                            attr->revision,
                            name,
                            attr->argTaskset,
                            priority,
                            attr->maxContention,
                            attr->enableClearLs);
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursCreateTask2_proxy(uint32_t taskset,
                                      uint32_t id,
                                      uint32_t eaElf,
                                      const CellSpursTaskArgument* arg,
                                      const CellSpursTaskAttribute2* attr) {
    std::string argStr;
    if (arg) {
        argStr = ssnprintf("{%08x, %08x, %08x, %08x}",
                           arg->u32[0],
                           arg->u32[1],
                           arg->u32[2],
                           arg->u32[3]);
    }
    std::string name = "NULL";
    if (attr && attr->name) {
        readString(g_state.mm, attr->name, name);
    }
    INFO(libs) << ssnprintf(
        "cellSpursCreateTask2_proxy(%x, %x, %x, %s, attr: %d, %x, %llx, \"%s\")",
        taskset,
        id,
        eaElf,
        argStr,
        attr ? (uint32_t)attr->revision : -1u,
        attr ? (uint32_t)attr->sizeContext : -1u,
        attr ? (uint32_t)attr->eaContext : -1u,
        name);
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursCreateTaskWithAttribute_proxy(uint32_t taskset,
                                                  uint32_t id,
                                                  const CellSpursTaskAttribute* attr) {
    INFO(libs) << ssnprintf(
        "cellSpursCreateTaskWithAttribute_proxy(%x, %x, attr: %d, %x, %llx)",
        taskset,
        id,
        attr->revision,
        attr->sizeContext,
        attr->eaContext);
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursEventFlagAttachLv2EventQueue_proxy(uint32_t ev) {
    INFO(libs) << ssnprintf("cellSpursEventFlagAttachLv2EventQueue_proxy(%x)", ev);
    return g_state.th->getGPR(3);
}

emu_void_t _cellSpursEventFlagInitialize_proxy(
    uint32_t unk,
    uint32_t taskset,
    uint32_t ev,
    CellSpursEventFlagClearMode clearMode,
    CellSpursEventFlagDirection direction) {
    INFO(libs) << ssnprintf("cellSpursEventFlagInitialize_proxy(%x, %x, %s, %s)",
                            taskset,
                            ev,
                            to_string(clearMode),
                            to_string(direction));
    return g_state.th->getGPR(3);
}

emu_void_t cellSyncQueueInitialize_proxy(uint32_t obj,
                                         uint32_t ptr_buffer,
                                         uint32_t buffer_size,
                                         uint32_t depth) {
    INFO(libs) << ssnprintf("cellSyncQueueInitialize_proxy(%x, %x, %x, %x)",
                            obj,
                            ptr_buffer,
                            buffer_size,
                            depth);
    return g_state.th->getGPR(3);
}

emu_void_t _cellSpursQueueInitialize_proxy(uint32_t taskset,
                                           uint32_t ea,
                                           uint32_t buffer,
                                           uint32_t size,
                                           uint32_t depth,
                                           CellSpursQueueDirection direction) {
    INFO(libs) << ssnprintf("cellSpursQueueInitialize_proxy(%x, %x, %x, %x, %x, %s)",
                            taskset,
                            ea,
                            buffer,
                            size,
                            depth,
                            to_string(direction));
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursEventFlagWait_proxy(uint32_t flag,
                                        const uint16_t* bits,
                                        CellSpursEventFlagWaitMode mode) {
    INFO(libs) << ssnprintf("cellSpursEventFlagWait_proxy(%x, %x, %s)",
                            flag,
                            *bits,
                            to_string(mode));
    return g_state.th->getGPR(3);
}

emu_void_t _cellSpursAttributeInitialize_proxy(uint32_t attr,
                                               uint32_t revision,
                                               uint32_t sdkVersion,
                                               uint32_t nSpus,
                                               int32_t spuPriority,
                                               int32_t ppuPriority,
                                               bool exitIfNoWork) {
    INFO(libs) << ssnprintf(
        "_cellSpursAttributeInitialize_proxy(%x, %x, %x, %d, %d, %d, %d)",
        attr,
        revision,
        sdkVersion,
        nSpus,
        spuPriority,
        ppuPriority,
        exitIfNoWork);
    return g_state.th->getGPR(3);
}

emu_void_t cellSpursAttributeSetSpuThreadGroupType_proxy(uint32_t attr,
                                                         SpuThreadGroup type) {
    INFO(libs) << ssnprintf("cellSpursAttributeSetSpuThreadGroupType_proxy(%s)",
                            to_string(type));
    return g_state.th->getGPR(3);
}
