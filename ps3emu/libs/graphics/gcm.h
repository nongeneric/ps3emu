#pragma once

#include "../sys_defs.h"
#include "../../constants.h"
#include "sysutil_sysparam.h"
#include <boost/endian/arithmetic.hpp>
#include <boost/context/all.hpp>
#include <vector>
#include "ps3emu/enum.h"

class Process;
class MainMemory;

inline constexpr uint32_t gcmResetCommandsSize = 0x1000;

struct TargetCellGcmContextData {
    boost::endian::big_uint32_t begin;
    boost::endian::big_uint32_t end;
    boost::endian::big_uint32_t current;
    boost::endian::big_uint32_t callback;
};

ENUM(MemoryLocation,
    (Main, 0),
    (Local, 1)
)

namespace emu {
namespace Gcm {

struct CellGcmConfig {
    boost::endian::big_uint32_t localAddress;
    boost::endian::big_uint32_t ioAddress;
    boost::endian::big_uint32_t localSize;
    boost::endian::big_uint32_t ioSize;
    boost::endian::big_uint32_t memoryFrequency;
    boost::endian::big_uint32_t coreFrequency;
};
    
struct CellGcmOffsetTable {
    boost::endian::big_uint32_t ioAddress;
    boost::endian::big_uint32_t eaAddress;
};

uint32_t sys_rsx_device_map(big_uint64_t* unk0, big_uint64_t* unk1, uint64_t unk2);
uint32_t sys_rsx_memory_allocate(big_uint32_t* context,
                                 big_uint64_t* local,
                                 uint64_t unk0,
                                 uint64_t unk1,
                                 uint64_t unk2,
                                 uint64_t unk3);
uint32_t sys_rsx_context_allocate(big_uint32_t* context,
                                  big_uint64_t* dma_control_lpar,
                                  big_uint64_t* driver_info_lpar,
                                  big_uint64_t* reports_lpar,
                                  uint32_t mem_ctx_id,
                                  uint64_t system_mode);
uint32_t sys_rsx_context_free(uint32_t context);
uint32_t sys_rsx_memory_free(uint32_t mem_ctx_id);
uint32_t sys_rsx_context_iomap(uint32_t rsx_ctx_id,
                               uint32_t local_offset,
                               uint32_t main_mem_ea,
                               uint32_t size,
                               uint64_t flags);
uint32_t sys_rsx_context_iounmap(uint32_t rsx_ctx_id,
                                 uint64_t local_offset,
                                 uint64_t size);
uint32_t sys_rsx_context_attribute(uint32_t rsx_ctx_id,
                                   uint32_t pkg_id,
                                   uint64_t arg_1,
                                   uint64_t arg_2,
                                   uint64_t arg_3,
                                   uint64_t arg_4);
uint32_t sys_rsx_attribute(
    uint64_t pkg_id, uint64_t arg_1, uint64_t arg_2, uint64_t arg_3, uint64_t arg_4);

}}

ps3_uintptr_t rsxOffsetToEa(MemoryLocation location, ps3_uintptr_t offset);
std::vector<uint16_t> serializeOffsetTable();
void deserializeOffsetTable(std::vector<uint16_t> const& vec);
