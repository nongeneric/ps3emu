#include "gcm.h"
#include "graphics.h"
#include "ps3emu/rsx/GcmConstants.h"
#include "../sys.h"
#include "ps3emu/constants.h"
#include "ps3emu/Process.h"
#include "ps3emu/rsx/Rsx.h"
#include "ps3emu/rsx/RsxContext.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/InternalMemoryManager.h"
#include "ps3emu/log.h"
#include "ps3emu/state.h"
#include "ps3emu/fileutils.h"
#include "ps3emu/utils/SpinLock.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <algorithm>
#include <array>
#include <cstddef>

using namespace boost::endian;

namespace emu {
namespace Gcm {

enum RsxIds {
    RSX_MEMORY_CONTEXT_ID = 0x5a5a5a5b,
    RSX_CONTEXT_ID = 0x55555554,
    RSX_DMA_CONTROL_LPAR = 0x40100000,
    RSX_DRIVER_INFO_LPAR = 0x40200000,
    RSX_REPORTS_LPAR = 0x40300000,
    RSX_CONTEXT_ATTR_SURFACE = 0x1,
    RSX_CONTEXT_ATTR_SECOND_VFREQUENCY = 0x108,
    RSX_CONTEXT_ATTR_DISPLAY_BUFFER = 0x104,
    RSX_CONTEXT_ATTR_TILE = 0x300,
    RSX_CONTEXT_ATTR_FLIP_STATUS = 0x10a,
    RSX_CONTEXT_ATTR_FLIP_MODE = 0x101,
    RSX_CONTEXT_ATTR_BIND_TILE = 0x300,
    RSX_CONTEXT_ATTR_ZCULL = 0x301,
};

struct CellGcmTileInfo {
    big_uint32_t tile;
    big_uint32_t limit;
    big_uint32_t pitch;
    big_uint32_t format;
};

struct CellGcmZcullInfo {
    big_uint32_t region;
    big_uint32_t size;
    big_uint32_t start;
    big_uint32_t offset;
    big_uint32_t status0;
    big_uint32_t status1;
};
  
struct OffsetTable {
    SpinLock _m;

    OffsetTable() {
        for (auto& i : ioAddress) {
            i = 0xffff;
        }

        for (auto& i : eaAddress) {
            i = 0xffff;
        }

        for (auto& i : mapPageCount) {
            i = 0;
        }
    }

    std::array<big_uint16_t, 0xbff> ioAddress;
    std::array<big_uint16_t, 511> eaAddress;
    std::array<uint16_t, 511> mapPageCount;

    uint32_t eaToOffset(uint32_t ea) {
        auto lock = boost::lock_guard(_m);
        return ((uint32_t)ioAddress[ea >> 20] << 20) | ((ea << 12) >> 12);
    }

    uint32_t offsetToEa(uint32_t offset) {
        auto lock = boost::lock_guard(_m);
        return ((uint32_t)eaAddress[offset >> 20] << 20) | ((offset << 12) >> 12);
    }

    void unmapEa(uint32_t ea) {
        INFO(rsx) << ssnprintf("unmapping ea %08x", ea);
        auto lock = boost::lock_guard(_m);
        ea >>= 20;
        auto io = ioAddress[ea];
        assert(io != 0xffff);
        auto& count = mapPageCount[io];
        assert(count);
        for (auto i = 0u; i < count; ++i) {
            ioAddress[ea + i] = 0xffff;
            eaAddress[io + i] = 0xffff;
        }
        count = 0;
    }

    void unmapOffset(uint32_t offset) {
        unmapEa(offsetToEa(offset));
    }

    void map(uint32_t ea, uint32_t io, uint32_t size) {
        INFO(rsx) << ssnprintf("mapping ea %08x to io %08x of size %08x", ea, io, size);
        assert((size & (DefaultMainMemoryPageSize - 1)) == 0);
        assert((ea & (DefaultMainMemoryPageSize - 1)) == 0);
        auto lock = boost::lock_guard(_m);
        auto pages = size / DefaultMainMemoryPageSize;
        auto ioIndex = io >> 20;
        auto eaIndex = ea >> 20;
        mapPageCount[ioIndex] = pages;
        for (auto i = 0u; i < pages; ++i) {
            ioAddress[eaIndex] = ioIndex;
            eaAddress[ioIndex] = eaIndex;
            ioIndex++;
            eaIndex++;
        }
    }
};

struct {
    OffsetTable offsetTable;
} emuGcmState;

uint32_t sys_rsx_device_map(big_uint64_t* unk0, big_uint64_t* unk1, uint64_t unk2) {
    EMU_ASSERT(*unk0 == 0);
    EMU_ASSERT(unk2 == 8);
    INFO(libs) << ssnprintf("sys_rsx_device_map(%llx, ?, %llx)", *unk0, unk2);
    *unk1 = 0x40000000u;
    return CELL_OK;
}

uint32_t sys_rsx_memory_allocate(big_uint32_t* context,
                                 big_uint64_t* local,
                                 uint64_t unk0,
                                 uint64_t unk1,
                                 uint64_t unk2,
                                 uint64_t unk3) {
    INFO(libs) << ssnprintf(
        "sys_rsx_memory_allocate(?, ?, %llx, %llx, %llx, %llx)",
        unk0,
        unk1,
        unk2,
        unk3);
    *context = RSX_MEMORY_CONTEXT_ID;
    *local = RsxFbBaseAddr;
    EMU_ASSERT(unk0 == 0xF900000u);
    EMU_ASSERT(unk1 == 0x80000u);
    EMU_ASSERT(unk2 == 0x300000u);
    EMU_ASSERT(unk3 == 0xf);
    return CELL_OK;
}

uint32_t sys_rsx_context_allocate(big_uint32_t* context,
                                  big_uint64_t* dma_control_lpar,
                                  big_uint64_t* driver_info_lpar,
                                  big_uint64_t* reports_lpar,
                                  uint32_t mem_ctx_id,
                                  uint64_t system_mode) {
    INFO(libs) << "sys_rsx_context_allocate(...)";
    EMU_ASSERT(mem_ctx_id == RSX_MEMORY_CONTEXT_ID);
    EMU_ASSERT(system_mode == 0x820);
    *context = RSX_CONTEXT_ID;
    *dma_control_lpar = RSX_DMA_CONTROL_LPAR;
    *driver_info_lpar = RSX_DRIVER_INFO_LPAR;
    *reports_lpar = RSX_REPORTS_LPAR;
    g_state.mm->store32(RSX_DRIVER_INFO_LPAR, 0x211, g_state.granule);

    sys_ipc_key_t key = 0x800000001111333ull;
    sys_event_queue_attr attr { 0 };
    attr.attr_protocol = SYS_SYNC_FIFO;
    strncpy(attr.name, "RsxIntr", SYS_SYNC_NAME_SIZE);
    sys_event_queue_t queueId;
    auto res = sys_event_queue_create(&queueId, &attr, key, 8);
    assert(res == CELL_OK); (void)res;

    g_state.mm->store32(RSX_DRIVER_INFO_LPAR + 0x12D0, queueId, g_state.granule);

    //g_state.rsx->setGcmContext(ioSize, ioAddress);
    g_state.rsx->init(queueId);

    auto rsxPrimaryDmaLabel = 3;
    g_state.rsx->setLabel(rsxPrimaryDmaLabel, 1, false);

    return CELL_OK;
}

uint32_t sys_rsx_context_free(uint32_t context) {
    INFO(libs) << ssnprintf("sys_rsx_context_free(%x)", context);
    return CELL_OK;
}

uint32_t sys_rsx_memory_free(uint32_t mem_ctx_id) {
    INFO(libs) << ssnprintf("sys_rsx_memory_free(%x)", mem_ctx_id);
    return CELL_OK;
}

uint32_t sys_rsx_context_iomap(uint32_t rsx_ctx_id,
                               uint32_t local_offset,
                               uint32_t main_mem_ea,
                               uint32_t size,
                               uint64_t flags) {
    INFO(libs) << ssnprintf(
        "sys_rsx_context_iomap(%x, %x, %x)", local_offset, main_mem_ea, size);
    auto ptr = g_state.rsx->context()->mainMemoryBuffer.mapped() + local_offset;
    g_state.mm->provideMemory(main_mem_ea, size, ptr);
    emuGcmState.offsetTable.map(main_mem_ea, local_offset, size);
    return CELL_OK;
}

uint32_t sys_rsx_context_iounmap(uint32_t rsx_ctx_id,
                                 uint64_t local_offset,
                                 uint64_t size) {
    // TODO: unmap the "provided Memory"
    emuGcmState.offsetTable.unmapOffset(local_offset);
    return CELL_OK;
}

uint32_t sys_rsx_context_attribute(uint32_t rsx_ctx_id,
                                   uint32_t pkg_id,
                                   uint64_t arg_1,
                                   uint64_t arg_2,
                                   uint64_t arg_3,
                                   uint64_t arg_4) {
    if (pkg_id == RSX_CONTEXT_ATTR_DISPLAY_BUFFER) {
        auto id = arg_1;
        auto height = arg_2;
        auto offset = arg_3;
        INFO(libs) << ssnprintf(
            "sys_rsx_context_attribute(RSX_CONTEXT_ATTR_DISPLAY_BUFFER, %x, %x, %x)",
            id,
            height,
            offset);
        g_state.rsx->setDisplayBuffer(id, offset, height);
    } else if (pkg_id == RSX_CONTEXT_ATTR_SURFACE) {
        EMU_ASSERT(arg_1 == 0x1000);
        EMU_ASSERT(arg_2 == 0x1000);
        EMU_ASSERT(arg_3 == 0);
        EMU_ASSERT(arg_4 == 0);
    } else if (pkg_id == RSX_CONTEXT_ATTR_FLIP_STATUS) {
        INFO(libs) << ssnprintf(
            "sys_rsx_context_attribute(RSX_CONTEXT_ATTR_FLIP_STATUS, %x, %x, %x)",
            arg_1,
            arg_2,
            arg_3);
        if (arg_1 == 0 && arg_3 == 0x80000000) {
            // init, do nothing
        } else if (arg_1 == 1 && arg_3 == 0x80000000) {
            // cellGcmSetFlipStatus
            g_state.rsx->setFlipStatus(false);
        } else if (arg_1 == 1 && arg_2 == 0x7fffffff) {
            // cellGcmResetFlipStatus
            g_state.rsx->setFlipStatus(true);
        } else {
            EMU_ASSERT(false);
        }
    } else if (pkg_id == RSX_CONTEXT_ATTR_FLIP_MODE) {
        INFO(libs) << ssnprintf(
            "sys_rsx_context_attribute(RSX_CONTEXT_ATTR_FLIP_MODE, %x, %x, %x)",
            arg_1,
            arg_2,
            arg_3);
    } else if (pkg_id == RSX_CONTEXT_ATTR_BIND_TILE) {
        INFO(libs) << ssnprintf(
            "sys_rsx_context_attribute(RSX_CONTEXT_ATTR_BIND_TILE, %x, %x, %x)",
            arg_1,
            arg_2,
            arg_3);
        // do nothing
    } else if (pkg_id == RSX_CONTEXT_ATTR_ZCULL) {
        INFO(libs) << ssnprintf(
            "sys_rsx_context_attribute(RSX_CONTEXT_ATTR_ZCULL, %x, %x, %x)",
            arg_1,
            arg_2,
            arg_3);
        // do nothing
    } else {
        EMU_ASSERT(false);
    }
    return CELL_OK;
}

uint32_t sys_rsx_attribute(uint64_t pkg_id,
                           uint64_t arg_1,
                           uint64_t arg_2,
                           uint64_t arg_3,
                           uint64_t arg_4) {
    INFO(libs) << ssnprintf("sys_rsx_attribute(%llx, %llx, %llx, %llx, %llx)",
                            pkg_id,
                            arg_1,
                            arg_2,
                            arg_3,
                            arg_4);
    return CELL_OK;
}

}}

ps3_uintptr_t rsxOffsetToEa(MemoryLocation location, ps3_uintptr_t offset) {
    if (location == MemoryLocation::Local)
        return RsxFbBaseAddr + offset;
    return emu::Gcm::emuGcmState.offsetTable.offsetToEa(offset);
}

void logOffsetTable() {
    INFO(rsx) << "offset table dump:";
    auto table = &emu::Gcm::emuGcmState.offsetTable;
    for (auto i = 0u; i < table->ioAddress.size(); ++i) {
        if (table->ioAddress[i] != 0xffff) {
            INFO(rsx) << ssnprintf("io: %04x", table->ioAddress[i]);
        }
    }
    
    for (auto i = 0u; i < table->eaAddress.size(); ++i) {
        if (table->eaAddress[i] != 0xffff) {
            INFO(rsx) << ssnprintf("ea: %04x", table->eaAddress[i]);
        }
    }
    
    for (auto i = 0u; i < table->mapPageCount.size(); ++i) {
        if (table->mapPageCount[i]) {
            INFO(rsx) << ssnprintf("count: %04x", table->mapPageCount[i]);
        }
    }
}

std::vector<uint16_t> serializeOffsetTable() {
    auto table = &emu::Gcm::emuGcmState.offsetTable;
    std::vector<uint16_t> res = { 0, 0, 0 };
    uint16_t ioAddressCount = 0, eaAddressCount = 0, pageCount = 0;
    for (auto i = 0u; i < table->ioAddress.size(); ++i) {
        if (table->ioAddress[i] != 0xffff) {
            res.push_back(i);
            res.push_back(table->ioAddress[i]);
            ioAddressCount++;
        }
    }
    
    for (auto i = 0u; i < table->eaAddress.size(); ++i) {
        if (table->eaAddress[i] != 0xffff) {
            res.push_back(i);
            res.push_back(table->eaAddress[i]);
            eaAddressCount++;
        }
    }
    
    for (auto i = 0u; i < table->mapPageCount.size(); ++i) {
        if (table->mapPageCount[i]) {
            res.push_back(i);
            res.push_back(table->mapPageCount[i]);
            pageCount++;
        }
    }
    
    res[0] = ioAddressCount;
    res[1] = eaAddressCount;
    res[2] = pageCount;
    
    return res;
}

void deserializeOffsetTable(std::vector<uint16_t> const& vec) {
    auto table = &emu::Gcm::emuGcmState.offsetTable;
    std::fill(begin(table->ioAddress), end(table->ioAddress), 0xffff);
    std::fill(begin(table->eaAddress), end(table->eaAddress), 0xffff);
    std::fill(begin(table->mapPageCount), end(table->mapPageCount), 0);
    
    unsigned pos = 3;
    for (auto i = 0u; i < vec[0]; ++i) {
        table->ioAddress[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
    for (auto i = 0u; i < vec[1]; ++i) {
        table->eaAddress[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
    for (auto i = 0u; i < vec[2]; ++i) {
        table->mapPageCount[vec[pos]] = vec[pos + 1];
        pos += 2;
    }
    for (auto i = 0u; i < table->mapPageCount.size(); ++i) {
        if (table->mapPageCount[i]) {
            auto io = i << 20;
            auto ea = (uint32_t)table->eaAddress[i] << 20;
            auto size = table->mapPageCount[i] * DefaultMainMemoryPageSize;
            auto ptr = g_state.rsx->context()->mainMemoryBuffer.mapped() + io;
            g_state.mm->provideMemory(ea, size, ptr);
        }
    }
}
