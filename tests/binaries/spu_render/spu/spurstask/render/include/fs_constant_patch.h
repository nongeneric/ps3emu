#ifndef SPURS_TASK_RENDER_FS_CONSTANT_PATCH_H
#define SPURS_TASK_RENDER_FS_CONSTANT_PATCH_H
#include <cell/gcm_spu.h>
#include "debug.h"
#include "spu_util.h"
#include "spu_render_common.h"
#include "spu_render_tag.h"

class FsConstantPatch : public FsConstantPatchContext{
public:
	FsConstantPatch(uint32_t ea_FsConstantPatchContext){
		if(m_instance){	MY_ASSERT(false); }
		m_instance = this;
		cellDmaLargeGet(this,ea_FsConstantPatchContext, sizeof(FsConstantPatchContext), TAG_FOR_INPUT_DATA, 0, 0);
	}

	inline static void unload(){
		cellDmaLargePut(m_instance,m_instance->getEa(), sizeof(FsConstantPatchContext), TAG_FOR_OUTPUT_DATA, 0, 0);
		m_instance = NULL;
	}

	inline static uint32_t sendFragmentProgram(CellGcmContextData* context, CGprogram fragmentShader){
		return m_instance->sendFragmentProgramInternal(context,fragmentShader);
	}

private:
	enum{
		RSX_CONSUMED = 0x00000000,
		SHADER_PRODUCED = 0xffffffff,
	};
	inline static uint32_t calcBufferSizeWithOverfetch(uint32_t microcodeSize){
		uint32_t eop = ((microcodeSize - 16) % 256) / 16 + 1;
		uint32_t overfetch = (16 - eop) * 16 + ((eop < 7) ? 0 : 256);
		return  microcodeSize + overfetch;
	}
	uint32_t sendFragmentProgramInternal(CellGcmContextData* context, CGprogram fragmentShader);
	void switchToNextSegment(CellGcmContextData* context);

	static FsConstantPatch* m_instance;
};

void patchFragmentProgramParameter(const CGprogram prog, const CGparameter param, const vec_float4* value);

#endif // SPURS_TASK_RENDER_FS_CONSTANT_PATCH_H