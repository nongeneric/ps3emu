#ifndef COMMON_SPU_RENDER_COMMON_H
#define COMMON_SPU_RENDER_COMMON_H

#include "base.h"
#include "debug.h"
#include "rsx_resource.h"

enum{
	ARG_SPU_RENDER_EA_INIT_DATA = 0,
	ARG_SPU_RENDER_RESERVED0 = 1,
	ARG_SPU_RENDER_RESERVED1 = 2,
	ARG_SPU_RENDER_DEBUG_FLAG = 3,
};

enum{
	DEBUG_FLAG_SPU_RENDER_NO_YIELD = 1,
	DEBUG_FLAG_SPU_RENDER_PROCESS_ONE_PACKET = (1 << 1),
};

enum{
	EXIT_CODE_SPU_RENDER_OK = CELL_OK,
	EXIT_CODE_SPU_RENDER_FAILURE = -1,
};

class SpuAddressToOffsetInitData{

public:
	enum{
		SPU_ADDRESS_TO_OFFSET_MAIN_MAP,
		SPU_ADDRESS_TO_OFFSET_LOCAL_MAP,
		SPU_ADDRESS_TO_OFFSET_MAP_NUM,
	};
	static const uint32_t SPU_ADDRESS_TO_OFFSET_INVALID = 0xffffffff;
#ifdef __PPU__
	inline void setMap(int index, spu_addr begin, spu_addr end, uint32_t offset){
		m_mapInfo[index].set(begin,end,offset);
	}
#endif // __PPU__
	inline uint32_t AddressToOffset(spu_addr addr){
		uint32_t ret = SPU_ADDRESS_TO_OFFSET_INVALID;
		for(uint32_t i=0; i < SPU_ADDRESS_TO_OFFSET_MAP_NUM; i++){
			uint32_t offset = m_mapInfo[i].AddressToOffset(addr);
			ret = (offset != SPU_ADDRESS_TO_OFFSET_INVALID) ? offset : ret;
		}
		return ret;
	}
#ifdef __PPU__
	uint32_t getEa(){return reinterpret_cast<uint32_t>(this);};
#endif // __PPU__

private:
	class MapInfo{
	public:
		inline uint32_t AddressToOffset(spu_addr addr){
			uint32_t ret = addr - m_begin_ea + m_base_offset;
			ret = (addr < m_begin_ea) ? SPU_ADDRESS_TO_OFFSET_INVALID : ret; // Check address range.
			ret = (addr >= m_end_ea) ? SPU_ADDRESS_TO_OFFSET_INVALID : ret; // Check address range.
			return ret;
		}
		inline void set(spu_addr begin, spu_addr end, uint32_t offset){
			m_begin_ea = begin;
			m_end_ea = end;
			m_base_offset = offset;
		}
	private:
		spu_addr m_begin_ea;
		spu_addr m_end_ea;
		uint32_t m_base_offset;
		uint32_t padding;
	} m_mapInfo[SPU_ADDRESS_TO_OFFSET_MAP_NUM];
} __attribute__ ((aligned(16)));

struct SpuCommandContextInitData{
	uint32_t ea_main_context;
	uint32_t offset_of_main_context_begin;
	uint32_t padding[2];
#ifdef __PPU__
	void setContext(CellGcmContextData* context){
		MY_ASSERT(context);
		ea_main_context = reinterpret_cast<uint32_t>(context);
		MY_ALIGN_CHECK16(ea_main_context);
		MY_C(cellGcmAddressToOffset(context->begin,&offset_of_main_context_begin));
	}
#endif // __PPU__
}__attribute__ ((aligned(16)));

class FsConstantPatchContext{
public:
	enum{
		SEGMENT_NUM = 8,
	};

#ifdef __PPU__
	inline int setupContext(
		void* ea_local_memory_buffer_start,
		uint32_t local_memory_buffer_size,
		uint32_t report_index_begin)
	{
		m_ea_local_memory_buffer_start = reinterpret_cast<uint32_t>(ea_local_memory_buffer_start);
		MY_C(cellGcmAddressToOffset((void*)m_ea_local_memory_buffer_start, &m_offset_local_memory_buffer_start));
		m_report_index_begin = report_index_begin;
		m_ea_main_memory_report_start = reinterpret_cast<uint32_t>(
			cellGcmGetReportDataAddressLocation(report_index_begin,CELL_GCM_LOCATION_MAIN));
		MY_ASSERT((m_ea_main_memory_report_start & 0x7f) == 0); // report_address_start should be 128byte align
		m_local_memory_segment_size = local_memory_buffer_size / SEGMENT_NUM;
		return CELL_OK;
	}
	inline void initParameters(){ setSegment(0); }
#endif 

	uint32_t getEa(){
#ifdef __PPU__  // In case FsConstantPatchContext moved, ea_of_this should be updated with getEa().
		m_ea_of_this = reinterpret_cast<uint32_t>(this);
#endif // __PPU__
		return m_ea_of_this;
	}

protected:
	static const uint32_t WAIT_COUNT = 1000;
	inline void setSegment(uint32_t segmentIndex){
		m_ea_segment_current = m_ea_local_memory_buffer_start + m_local_memory_segment_size * segmentIndex;
		m_ea_segment_end = m_ea_segment_current + m_local_memory_segment_size;
		m_segment_index = segmentIndex;
	}

	inline uint32_t getOffset(uint32_t addr){
		MY_ASSERT(addr >= m_ea_local_memory_buffer_start);
		return m_offset_local_memory_buffer_start + (addr - m_ea_local_memory_buffer_start);
	}

	inline uint32_t getSegmentIndex(){return m_segment_index;}
	inline uint32_t getEaSegmentCurrent(){return m_ea_segment_current;}
	inline void setEaSegmentCurrent(uint32_t current){ m_ea_segment_current = current;}
	inline uint32_t getEaSegmentEnd(){return m_ea_segment_end;}
	inline uint16_t getReportIndexBegin(){return m_report_index_begin;}
	inline uint32_t getEaMainMemoryReportStart(){return m_ea_main_memory_report_start;}

private:
	// Initial parameter
	uint32_t m_ea_local_memory_buffer_start;
	uint32_t m_offset_local_memory_buffer_start;
	uint32_t m_local_memory_segment_size;
	uint32_t m_ea_main_memory_report_start;
	uint16_t m_report_index_begin;
	// Update parameter
	uint16_t m_segment_index;
	uint32_t m_ea_segment_current;
	uint32_t m_ea_segment_end;
	// Ea for Unload
	uint32_t m_ea_of_this;

} __attribute__ ((aligned(16)));

struct SpuRenderInitData{
	struct SpuRenderInitDataInOut{
		uint32_t lastJtsAddr;
		uint32_t ea_current_packet;
		uint32_t padding[2];
	}inout;
	struct SpuRenderInitDataIn{
		uint32_t ea_FsConstantPatchContext;
		uint32_t ea_RenderEnv;
		uint32_t ea_SpuAddressToOffsetInitData;
		uint32_t padding;
		SpuCommandContextInitData spuCommandContextInitData;
	}in;
#ifdef __PPU__
	uint32_t getEa(){return reinterpret_cast<uint32_t>(this);};
#endif // __PPU__
}__attribute__ ((aligned(16)));


#endif // COMMON_SPU_RENDER_COMMON_H