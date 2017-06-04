#ifndef SPURS_TASK_RENDER_DMA_CACHE_H
#define SPURS_TASK_RENDER_DMA_CACHE_H

#include "spu_util.h"

class DmaCache
{
public:
	DmaCache(void) : 
	  m_ea_fetched_vertex_shader(0), m_ea_fetched_fragment_shader(0),
		  m_drawpacket_end(DMA_CACHE_FALSE), m_drawpacket_current_index(0),
		  m_ea_current_material(0), m_material_current_index(0), m_material_is_update(DMA_CACHE_FALSE),
		  m_work_current_index(0), m_work_is_update(DMA_CACHE_FALSE){}
	~DmaCache(void) {}
	inline void dmaShader(const ShaderInfo& vertex, const ShaderInfo& fragment){
		//J ここでは、前にdmaしたアドレスを覚えておき同じeaの物がきたら転送を行わない
		if(m_ea_fetched_vertex_shader != vertex.ea_ShaderBinary){
			m_ea_fetched_vertex_shader = vertex.ea_ShaderBinary;
			cellDmaLargeGet(m_vertexShaderBuffer.getLsAddress(), vertex.ea_ShaderBinary, vertex.binary_size, TAG_FOR_VERTEX_SHADER, 0, 0);
		}

		if(m_ea_fetched_fragment_shader != fragment.ea_ShaderBinary){
			m_ea_fetched_fragment_shader = fragment.ea_ShaderBinary;
			cellDmaLargeGet(m_fragmentShaderBuffer.getLsAddress(), fragment.ea_ShaderBinary, fragment.binary_size, TAG_FOR_FRAGMENT_SHADER, 0,0);
		}
	}

	inline CGprogram getFragmentShader(){return m_fragmentShaderBuffer.getPtr();}
	inline CGprogram getVertexShader(){return m_vertexShaderBuffer.getPtr();}
	inline DrawPacketNode* getCurrentDrawPacket(){ return m_drawpacketBuffer[getCurrentDrawPacketIndex()].getPtr(); }
	inline Material* getCurrentMaterial(){ return m_materialBuffer[getCurrentMaterialIndex()].getPtr(); }
	inline Work* getCurrentWork(){ return m_workBuffer[getCurrentWorkIndex()].getPtr(); }

	inline void updateIndex(){
		m_drawpacket_current_index = getNextDrawPacketIndex();
		m_work_current_index	= (m_work_is_update) ? getNextWorkIndex() : getCurrentWorkIndex();
		m_material_current_index = (m_material_is_update) ? getNextMaterialIndex() : getCurrentMaterialIndex();
	}
	inline void sendFirstPrefetchRequest(uint32_t ea_drawpacket){
		cellDmaLargeGet(m_drawpacketBuffer[0].getLsAddress(), ea_drawpacket, sizeof(DrawPacketNode), TAG_FOR_DMA_CACHE, 0, 0);
	}
	inline void sendSecondPrefetchRequest(){
		sendRequestInternal(0 /* fetchedPacket */, 1 /* writePacketIndex */, 0  /* writeMaterialIndex */, 0 /*writeWorkIndex*/);
	}
	inline void sendPrefetchRequest(){
		sendRequestInternal(getNextDrawPacketIndex(),getAfterTheNextDrawPacketIndex(), getNextMaterialIndex(), getNextWorkIndex());
	}

private:
	enum{
		DMA_CACHE_TRUE = 1,
		DMA_CACHE_FALSE = 0,
		DRAWPACKET_PREFETCH_NUM = 3,
		MATERIAL_PREFETCH_NUM = 2,
		WORK_PREFETCH_NUM = 2,
	};
	static const uint32_t MATERIAL_BUFFER_SIZE = 16 * KiB;
	static const uint32_t SHADER_BUFFER_SIZE = 16 * KiB;

	SpuBuffer<_CGprogram, SHADER_BUFFER_SIZE> m_vertexShaderBuffer __attribute__ ((aligned(SHADER_BINARY_ALIGNMENT)));
	SpuBuffer<_CGprogram, SHADER_BUFFER_SIZE> m_fragmentShaderBuffer __attribute__ ((aligned(SHADER_BINARY_ALIGNMENT)));
	SpuBuffer<Material, MATERIAL_BUFFER_SIZE> m_materialBuffer[MATERIAL_PREFETCH_NUM] __attribute__ ((aligned( __alignof__(Material))));
	SpuBuffer<Work> m_workBuffer[WORK_PREFETCH_NUM] __attribute__ ((aligned( __alignof__(Work))));
	SpuBuffer<DrawPacketNode> m_drawpacketBuffer[DRAWPACKET_PREFETCH_NUM] __attribute__ ((aligned( __alignof__(DrawPacketNode))));

	uint32_t m_ea_fetched_vertex_shader;
	uint32_t m_ea_fetched_fragment_shader;
	uint32_t padding0[2];

	uint32_t m_drawpacket_end;
	uint32_t m_drawpacket_current_index;
	uint32_t m_ea_current_material;
	uint32_t padding1;

	uint32_t m_material_current_index;
	uint32_t m_material_is_update;
	uint32_t m_work_current_index;
	uint32_t m_work_is_update;

	inline void sendRequestInternal(uint32_t fetchPacketIndex, uint32_t writePacketIndex, uint32_t writeMaterialIndex, uint32_t writeWorkIndex ){
		cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_DMA_CACHE));
		if(m_drawpacket_end == DMA_CACHE_TRUE)
			return;
		DrawPacketNode* fetchedPacket = m_drawpacketBuffer[fetchPacketIndex].getPtr();
		// DrawPacket
		uint32_t ea_next_packet = reinterpret_cast<uintptr_t>(fetchedPacket->getNext());
		if(ea_next_packet){
			cellDmaLargeGet(m_drawpacketBuffer[writePacketIndex].getLsAddress(),
				ea_next_packet, sizeof(DrawPacketNode), TAG_FOR_DMA_CACHE, 0, 0);
		}
		else{
			m_drawpacket_end = DMA_CACHE_TRUE;
		}
		m_work_is_update = m_material_is_update = DMA_CACHE_FALSE;
		if(fetchedPacket->getType() == DrawPacketBase::TYPE_DRAW_COMMAND){
			// Material
			DrawPacket* drawPacket = fetchedPacket->getDrawPacket();
			if(m_ea_current_material != drawPacket->getEaMaterial()){
				m_ea_current_material = drawPacket->getEaMaterial();
				m_material_is_update = DMA_CACHE_TRUE;
				cellDmaLargeGet(m_materialBuffer[writeMaterialIndex].getLsAddress(),
					drawPacket->getEaMaterial(), drawPacket->getMaterialSize(), TAG_FOR_DMA_CACHE, 0, 0);
			}
			// Work
			m_work_is_update = DMA_CACHE_TRUE;
			cellDmaLargeGet(m_workBuffer[writeWorkIndex].getLsAddress(),
				drawPacket->getEaWork(), sizeof(Work), TAG_FOR_DMA_CACHE, 0, 0);
		}
	}

	inline uint32_t getCurrentDrawPacketIndex(){ return m_drawpacket_current_index; }
	inline uint32_t getNextDrawPacketIndex(){return (m_drawpacket_current_index + 1) % DRAWPACKET_PREFETCH_NUM;}
	inline uint32_t getAfterTheNextDrawPacketIndex(){return (m_drawpacket_current_index + 2) % DRAWPACKET_PREFETCH_NUM;}
	inline uint32_t getCurrentMaterialIndex(){ return m_material_current_index; }
	inline uint32_t getNextMaterialIndex(){return (m_material_current_index + 1) % MATERIAL_PREFETCH_NUM;}
	inline uint32_t getCurrentWorkIndex(){ return m_work_current_index; }
	inline uint32_t getNextWorkIndex(){return (m_work_current_index + 1) % WORK_PREFETCH_NUM;}
};

#endif // SPURS_TASK_RENDER_DMA_CACHE_H2