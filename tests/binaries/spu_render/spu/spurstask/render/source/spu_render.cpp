#include <new> // For placement new
#include <cell/spurs.h>
#include "debug.h"
#include <spu_printf.h>
#include "fs_constant_patch.h"
#include "spu_command_context.h"
#include "shader_common.h"
#include "render_state_cache.h"
#include "spu_render.h"
#include <spu_mfcio.h>

// Static value
uint32_t SpuRender::decremneter = 0;


void SpuRender::prepare(uint32_t _ea_SpuRenderInitData){
	ea_SpuRenderInitData = _ea_SpuRenderInitData;

	//J Load InitData
	cellDmaLargeGet(&init_data,ea_SpuRenderInitData,sizeof(init_data),TAG_FOR_INPUT_DATA,0,0);
	cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_INPUT_DATA));
	
	//J Setup Spu Command Buffer
	context.setupFromInitData(init_data.in.spuCommandContextInitData);
	
	//J Setup FragmentShader Constant Patch
	static SpuBuffer<FsConstantPatch> fsConstantPatch;
	new(fsConstantPatch.getLsAddress())FsConstantPatch(init_data.in.ea_FsConstantPatchContext);

	SpuAddressToOffset::load(init_data.in.ea_SpuAddressToOffsetInitData);
	cellDmaLargeGet(&render_env,init_data.in.ea_RenderEnv, sizeof(render_env), TAG_FOR_INPUT_DATA, 0, 0);
	//J 最初のDrawPacketのリクエストをする。
	dmaCache.sendFirstPrefetchRequest(getCurrentPacketEa());
	
	cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_INPUT_DATA));
}

void SpuRender::quit(){
	//J Unload InitData
	cellDmaLargePut(&init_data.inout, ea_SpuRenderInitData, sizeof(init_data.inout), TAG_FOR_OUTPUT_DATA, 0, 0);
	//J Unload Spu Command Buffer
	context.unloadMainContext();
	//J Unload FragmentShaderConstant Patch
	FsConstantPatch::unload();
}

void SpuRender::processDrawCommand(DrawPacket* packet){
	Material* material = dmaCache.getCurrentMaterial();
	Work* work = dmaCache.getCurrentWork();
	Transform* transform = &work->transform;
	//J Shaderはサイズが大きいのでPrefetchを行っていない
	//J Fragment Shader Bufferが更新される前にTAG_FOR_GPU_SYNCの完了を待ちます。
	cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_GPU_SYNC));
	dmaCache.dmaShader(material->vertex_shader_info, material->fragment_shader_info);

	context.Align(); //J Note: SIMDでコマンドバッファを作成するためにアライメントをあわせる
	CellGcmContextData* raw_context = reinterpret_cast<CellGcmContextData*>(&context);
	//J <Set Vertex Info>
	uint32_t* current = raw_context->current;
	material->vertex_info.SetVertexDataArray(raw_context,true);
	uint32_t* newCurrent = raw_context->current;


	//J <Set Textures>
	context.ReserveMethodSize(TextureInfo::getCommandSize() * material->texture_number);
	for(uint8_t i=0; i < material->texture_number; i++){
		TextureInfo* texInfo = &(material->texture_info[i]);
		uint32_t ea_texture_info = reinterpret_cast<uintptr_t>(texInfo) - reinterpret_cast<uintptr_t>(material) + packet->getEaMaterial();
		texInfo->SetTexture(raw_context, RenderStateCache::setTexture(i, ea_texture_info)); // Method Macro SIMD版
	}

	//J <Set Vertex Shader Common Parameters>
	//J Note: 行列を設定すべきIndexを事前に固定化しているため、VertexShaderの転送を待たずに設定を行える
	Vectormath::Aos::Matrix4 Model = transform->getTransformMatrix();
	Vectormath::Aos::Matrix4 ProjViewModel = render_env.ProjView * Model;
	Vectormath::Aos::Matrix4 ViewModel = render_env.View * Model;
	context.ReserveMethodSize(context.GetVertexProgramConstantsMatrix4x4AosSize() * 2);
	context.SetVertexProgramConstantsMatrix4x4Aos(VERTEXSHADER_MODEL_VIEW_PROJ_CONSTANT_INDEX, 
		reinterpret_cast<const float*>(&ProjViewModel), true); // Method Macro SIMD版
	context.SetVertexProgramConstantsMatrix4x4Aos(VERTEXSHADER_MODEL_VIEW_CONSTANT_INDEX,
		reinterpret_cast<const float*>(&ViewModel), true); // Method Macro SIMD版

	//J <Set Vertex Shader>
	cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_VERTEX_SHADER));
	CGprogram vertexShader = dmaCache.getVertexShader();
	if(RenderStateCache::setVertexShader(material->vertex_shader_info.ea_ShaderBinary) == RenderStateCache::CACHE_MISS)
		context.SetVertexProgram(vertexShader); // Method Macro SIMD版
	
	//J <Set Fragment Shader>
	cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_FRAGMENT_SHADER));
	CGprogram fragmentShader = dmaCache.getFragmentShader();
	//J Parameterの更新
	{
		CGparameter param = cellGcmCgGetNamedParameter(fragmentShader,DIRECTIONAL_LIGHT_DIR_NAME);
		if(param != 0)
			patchFragmentProgramParameter(fragmentShader, param, &render_env.directional_light_dir);
	}
	{
		CGparameter param = cellGcmCgGetNamedParameter(fragmentShader,DIRECTIONAL_LIGHT_COLOR_NAME);
		if(param != 0){
			vec_float4 light_color = (work->isOverwrite) ? work->overwrite_light_color : render_env.directional_light_color;
			patchFragmentProgramParameter(fragmentShader, param, &light_color);
		}
	}
	//J 更新したFragmentShaderをLocal Memory(VRAM)へ転送し、そのoffsetアドレスを取得する。
	uint32_t offset = FsConstantPatch::sendFragmentProgram(raw_context,fragmentShader);
	//J Note: RenderStateCacheを更新し、同じFragmentShaderであれば再設定処理をキャンセルする
	if(RenderStateCache::setFragmentShader(material->fragment_shader_info.ea_ShaderBinary) == RenderStateCache::CACHE_MISS)
		context.SetFragmentProgram(fragmentShader,offset);
	else
		context.SetUpdateFragmentProgramParameterLocation(offset,CELL_GCM_LOCATION_LOCAL);

	// <Draw Index Array>
	context.SetDrawIndexArray(material->vertex_info.index_mode,
		material->vertex_info.index_count, 
		material->vertex_info.index_type, 
		material->vertex_info.index_location, 
		material->vertex_info.index_offset);
}

void SpuRender::processPacket(DrawPacketNode* packet)
{
	uint32_t nowHeadAddr = 0; //J DrawPacketが生成するコマンドバッファの先頭EA
	uint32_t nowJtsAddr	= 0; //J  DrawPacketが生成したJTSのEA
	MY_ASSERT(packet->checkDebugBit() == 0);
	switch(packet->getType()){
		case DrawPacketBase::TYPE_DRAW_COMMAND:
		{
			DrawPacket* drawPacket = packet->getDrawPacket();
			nowHeadAddr = context.getMainContextCurrentEa();
			processDrawCommand(drawPacket); //J renderingコマンドを作成する
			nowJtsAddr = context.SetJumpToSelf(); // JTSコマンドを作成する。
			//J LS Contextに残っているコマンドをTAG_FOR_GPU_SYNCですべて書き出します。
			context.finishContext();
			break;
		}
		case DrawPacketBase::TYPE_CALL_PREBUILD_COMMAND:
		{
			CallPacket* callPacket = packet->getCallPacket();
			nowHeadAddr = context.getMainContextCurrentEa();
			context.SetCallCommand(callPacket->getOffsetPrebuildCommandHead());
			nowJtsAddr = context.SetJumpToSelf(); //J JTSコマンドを作成する。
			//J LS Contextに残っているコマンドをTAG_FOR_GPU_SYNCですべて書き出します。
			//J context内で前に発行したJTSについてTAG_FOR_GPU_SYNCの完了は待っています。
			context.finishContext();
			RenderStateCache::reset();
			break;
		}
		case DrawPacketBase::TYPE_JUMP_PREBUILD_COMMAND:
		{
			JumpPacket* jumpPacket = packet->getJumpPacket();
			nowHeadAddr = jumpPacket->getEaPrebuildCommandHead();
			nowJtsAddr = jumpPacket->getEaPrebuildCommandJts();
			//J Note: RenderStateはJump Contextの中で更新される可能性があるので、resetを行う。
			RenderStateCache::reset();
			//J TAG_FOR_GPU_SYNCの完了は待っています。
			cellDmaWaitTagStatusAll(TAG_BIT(TAG_FOR_GPU_SYNC));
			break;
		}
		default:
			MY_ASSERT(false);
	}
	//J TAG_FOR_GPU_SYNCでJTSを解除します。
	//J Note: TAG_FOR_GPU_SYNCで書き出すことで、コマンドバッファと定数パッチの書き込み終了が保証されます。
	releaseJTStoJTN(getLastJtsAddr(), nowHeadAddr); 
	//J 現在止めているのJTSのEAアドレスをセットする。
	setLastJtsAddr(nowJtsAddr);
}

void SpuRender::run(){
	SpuRender::decremneter = spu_read_decrementer();
	dmaCache.sendSecondPrefetchRequest();
	RenderStateCache::reset();
	context.initContext();
	while(getCurrentPacketEa()){
		//J Prefetch dma requestを発行する。
		dmaCache.sendPrefetchRequest();
		//J fetch済みパケットを取得する。
		DrawPacketNode* packet = dmaCache.getCurrentDrawPacket();
		processPacket(packet);
		setCurrentPacketEa(packet->getNext());
		//J dmaCacheのIndexを進める
		dmaCache.updateIndex();
		{
			uint32_t current = spu_read_decrementer();
			uint32_t delta = (SpuRender::decremneter > current) ? SpuRender::decremneter - current : SpuRender::decremneter + (0xffffffff - current);
			if(delta > SpuRender::kTaskPollCycleInDecrementerCount){
				MY_LSGUARD_CHECK();
				if(cellSpursTaskPoll2()){
					cellSpursYield();
					 //J Yieldから戻ってきたとき、decrementerの値が大きく変わっているため再度読み込む
					SpuRender::decremneter = spu_read_decrementer();
				}
				else
					SpuRender::decremneter = current;
				MY_LSGUARD_REHASH();
			}
		}
	}
}

int cellSpursTaskMain(qword argTask, uint64_t argTaskset){
	MY_LSGUARD_REHASH();
	static SpuRender render;
	vec_uint4 arg_vec_ui4 = reinterpret_cast<vec_uint4>(argTask);
	render.prepare(arg_vec_ui4[ARG_SPU_RENDER_EA_INIT_DATA]);
	render.run();
	render.quit();
	MY_LSGUARD_CHECK();
	return 0;
}
