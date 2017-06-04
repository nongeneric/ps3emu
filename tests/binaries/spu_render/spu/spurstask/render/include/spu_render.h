#ifndef SPURS_TASK_RENDER_SPU_RENDER_H
#define SPURS_TASK_RENDER_SPU_RENDER_H

#include "unit.h"
#include "data.h"
#include "spu_render_common.h"
#include "spu_render_tag.h"
#include "spu_command_context.h"
#include "shader_common.h"
#include "dma_cache.h"

class SpuRender{
public:
	SpuRender(){};
	~SpuRender(){};
	void prepare(uint32_t ea_SpuRenderInitData);
	void run();
	void quit();
	static uint32_t decremneter;
	static const uint32_t kDecrementerCycleInOneUsec = 80;
	static const uint32_t kTaskPollCycleInUsec = 250;
	static const uint32_t kTaskPollCycleInDecrementerCount = kTaskPollCycleInUsec * kDecrementerCycleInOneUsec;
private:
	SpuRenderInitData init_data;
	SpuCommandContext context;
	RenderEnv render_env;
	DmaCache dmaCache;
	uint32_t ea_SpuRenderInitData;

	inline void setLastJtsAddr(uint32_t jtsAddr){init_data.inout.lastJtsAddr = jtsAddr;};
	inline uint32_t getLastJtsAddr(){return init_data.inout.lastJtsAddr;};
	inline uint32_t getCurrentPacketEa(){ return init_data.inout.ea_current_packet; };
	inline void setCurrentPacketEa(DrawPacketNode* packet){
		init_data.inout.ea_current_packet = reinterpret_cast<uintptr_t>(packet);
	};

	void processPacket(DrawPacketNode* packet);
	void processDrawCommand(DrawPacket* packet);
};

#endif // SPURS_TASK_RENDER_SPU_RENDER_H