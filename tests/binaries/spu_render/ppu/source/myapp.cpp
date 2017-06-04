/*  SCE CONFIDENTIAL
*  PlayStation(R)3 Programmer Tool Runtime Library 400.001
*  Copyright (C) 2010 Sony Computer Entertainment Inc.
*  All Rights Reserved.
*/

#include <sys/memory.h>
#include <sys/synchronization.h>
#include <sysutil/sysutil_sysparam.h>
#include <sysutil/sysutil_common.h>

#include <stdlib.h>
#include <cell/gcm.h>
#include <cell/spurs.h>
#include <spu_printf.h>
#include <cstring>
#include "util.h"
#include "debug.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "mesh_manager.h"

#include "spu_render_common.h"
#include "shader_common.h"
#include "data.h"
#include "view.h"
#include "myapp.h"

// Spurs Task
extern const CellSpursTaskBinInfo _binary_task_spurs_task_render_elf_taskbininfo;

sys_event_flag_t MyApp::flip_event_flag;
uint8_t MyApp::buffer_index;

void MyApp::initBuffer(){
	// Allocate available memory
	sys_memory_info_t mem_info;
	MY_C(sys_memory_get_user_memory_size(&mem_info));
	MY_DPRINTF("Available Memory %d\n",mem_info.available_user_memory);
	sys_addr_t mainMemoryBegin;
	size_t mainMemorySize = (mem_info.available_user_memory - RESERVE_FOR_GPAD) & ~(MiB - 1);
	MY_C(sys_memory_allocate(mainMemorySize, SYS_MEMORY_PAGE_SIZE_1M, &mainMemoryBegin));
	
	// Setup mainMemoryHeap
	Sys::Memory::HeapBase& mainMemoryHeap = Sys::Memory::getMainMemoryHeap();
	mainMemoryHeap.init(reinterpret_cast<void*>(mainMemoryBegin),mainMemorySize);
	
	void* begin = mainMemoryHeap.alloc(MAPPED_MEMORY_SIZE, MiB);
	MY_C(cellGcmInit(MINIMUM_CB_SIZE,MAPPED_MEMORY_SIZE,begin));
	Sys::Memory::VramHeap& mappedMainMemoryHeap = Sys::Memory::getMappedMainMemoryHeap();
	mappedMainMemoryHeap.init(begin,MAPPED_MEMORY_SIZE);
	mappedMainMemoryHeap.alloc(MINIMUM_CB_SIZE); // Skip GCM Reserved area
	// Allocate Prebuild context buffer
	begin = mappedMainMemoryHeap.alloc(PRE_BUILD_CONTEXT_SIZE);
	cellGcmSetupContextData(&preBuildContext, reinterpret_cast<uint32_t*>(begin), PRE_BUILD_CONTEXT_SIZE, CommandContextAssertCallback);

	for(uint32_t i=0; i < VIDEO_BUFFER_NUM; i++){
		begin = mappedMainMemoryHeap.alloc(FLIP_CONTEXT_SIZE);
		cellGcmSetupContextData(&flipContext[i], reinterpret_cast<uint32_t*>(begin), FLIP_CONTEXT_SIZE, CommandContextAssertCallback);
	}

	// Allocate Main context buffer
	begin = mappedMainMemoryHeap.alloc(MAIN_CONTEXT_SIZE);
	cellGcmSetupContextData(&mainContext, reinterpret_cast<uint32_t*>(begin), MAIN_CONTEXT_SIZE, CommandContextAssertCallback);
	
	// Allocate Report date buffer
	report_begin = reinterpret_cast<CellGcmReportData*>(mainMemoryHeap.alloc(MAPPED_MEMORY_FOR_REPORT_SIZE, MiB));
	uint32_t report_buffer_align_size = ALIGN(MAPPED_MEMORY_FOR_REPORT_SIZE,MiB);
	MY_C(cellGcmMapEaIoAddress(report_begin, MAIN_REPORT_OFFSET_BEGIN, report_buffer_align_size));
	
	// Setup Local memory
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);
	Sys::Memory::getLocalMemoryHeap().init(config.localAddress, config.localSize);
	
	// Setup Temporary memory
	begin = mainMemoryHeap.alloc(TEMPORARY_BUFFER_SIZE, MiB);
	Sys::Memory::getTemporaryHeap().init(begin, TEMPORARY_BUFFER_SIZE);

	// Setup Frame memory
	begin = mainMemoryHeap.alloc(FRAME_BUFFER_SIZE, MiB);
	frameHeap[0].init(begin, FRAME_BUFFER_SIZE);
	begin = mainMemoryHeap.alloc(FRAME_BUFFER_SIZE, MiB);
	frameHeap[1].init(begin, FRAME_BUFFER_SIZE);
}

void MyApp::initSpurs(){
	spu_printf_initialize(1000,NULL);
	// Create Spurs
	spurs2 = Sys::Memory::getMainMemoryHeap().alloc<CellSpurs2>();
	CellSpursAttribute attributeSpurs;
	cellSpursAttributeInitialize(&attributeSpurs,
		SPURS_NUM_SPU,
		SPURS_SPU_THREAD_GROUP_PRIORITY,
		SPURS_HANDLER_THREAD_PRIORITY,
		SPURS_RELEASE_SPU_WHEN_IDLE);
	cellSpursAttributeEnableSpuPrintfIfAvailable(&attributeSpurs);
	MY_C(cellSpursInitializeWithAttribute2(spurs2,&attributeSpurs));

	// Create Taskset
	taskset = Sys::Memory::getMainMemoryHeap().alloc<CellSpursTaskset2>();
	CellSpursTasksetAttribute2 attributeTaskset;
	cellSpursTasksetAttribute2Initialize(&attributeTaskset);
	attributeTaskset.name = "BASIC_TASKSET";
	attributeTaskset.argTaskset = 0;
	const uint8_t priorities[8] = {1,0,0,0,0,0,0,0};
	std::memcpy(attributeTaskset.priority, priorities, sizeof(priorities));
	attributeTaskset.maxContention = 1;
	MY_C(cellSpursCreateTaskset2(spurs2, taskset, &attributeTaskset));

	// Prepare Spurs_task_render context
	render_task_context_buffer = Sys::Memory::getMainMemoryHeap().alloc(
		_binary_task_spurs_task_render_elf_taskbininfo.sizeContext, SPURS_CONTEXT_BUFFER_ALIGN);
}

void MyApp::quitSpurs(){
	cellSpursDestroyTaskset2(taskset);
	MY_C(cellSpursFinalize(spurs2));
}

void MyApp::initVideoOut(){
	enum{
		WAIT_FOR_EVENT_ENABLE = 1,
		WAIT_FOR_EVENT_DISABLE = 0,
	};

	const uint8_t resolutionId	= CELL_VIDEO_OUT_RESOLUTION_720;
	const uint8_t aspect		= CELL_VIDEO_OUT_ASPECT_16_9;

	int isAvailable = cellVideoOutGetResolutionAvailability(CELL_VIDEO_OUT_PRIMARY, resolutionId, aspect, 0);

	MY_ASSERT(isAvailable);
	MY_C(cellVideoOutGetResolution(resolutionId,&resolution));
			
	CellVideoOutConfiguration videocfg;
	std::memset(&videocfg, 0, sizeof(CellVideoOutConfiguration)); // Set CellVideoOutConfiguration reserved[9] as 0
	videocfg.resolutionId = resolutionId;
	videocfg.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8; // We use A8R8G88B8 format.
	videocfg.pitch = getVideoWidth() * Render::getColorFormatSize(CELL_GCM_SURFACE_A8R8G8B8);
	videocfg.aspect = aspect;
	MY_C(cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &videocfg, NULL, WAIT_FOR_EVENT_ENABLE));
	
	// Validate configuration
	CellVideoOutState videoState;
	MY_C(cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState));
	if(videoState.displayMode.aspect != aspect || videoState.displayMode.resolutionId != resolutionId){
		MY_DPRINTF("This application works only for 16:9 720p resolution!\n");
		MY_ASSERT(0);
	}
	
	// Allocate video output buffer
	Render::ColorSurface cf;
	for (uint32_t i = 0; i < VIDEO_BUFFER_NUM; i++) {
		Render::createColorBuffer(cf, CELL_GCM_SURFACE_PITCH, getVideoWidth(), getVideoHeight(),
			CELL_GCM_SURFACE_A8R8G8B8, CELL_GCM_LOCATION_LOCAL, CELL_GCM_SURFACE_CENTER_1, true, CELL_GCM_COMPMODE_DISABLED, 0 );
		video_offset[i] = cf.offset;
		// regist surface
		MY_C(cellGcmSetDisplayBuffer(i, getVideoOffset(i), cf.pitch, getVideoWidth(), getVideoHeight()));
	}
	Render::bindDepthColorSurface(&cf,NULL,0,0,videoSurface);
	cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);

	//J Note: cellGcmGetFlipStatus()をポーリングしたくないので、Event Flagによる同期を行う
	sys_event_flag_attribute_t flag_attr;
	sys_event_flag_attribute_initialize(flag_attr);
	sys_event_flag_attribute_name_set(flag_attr.name, "FLIP");
	MY_C(sys_event_flag_create(&flip_event_flag, &flag_attr, 0));
	cellGcmSetFlipHandler(FlipHandler);
}

void MyApp::quitVideoOut(){
	cellGcmSetFlipHandler(NULL); //J FlipHandlerを解除する
	MY_C(sys_event_flag_destroy(flip_event_flag));
}

void MyApp::FlipHandler(const uint32_t head){
	sys_event_flag_set(flip_event_flag, FLIP_EVENT_FLAG_FLIP_DONE);
}

void MyApp::initRenderSurfaces(){

	// Reserve Normal Buffer MSAA 2x 1280 x 720 RGBA8
	MY_C(Render::createColorBuffer(colorBuffer2xMSAA, CELL_GCM_SURFACE_PITCH,
				RENDER_SURFACE_WIDTH, RENDER_SURFACE_HEIGHT,
				CELL_GCM_SURFACE_A8R8G8B8, CELL_GCM_LOCATION_LOCAL,
				CELL_GCM_SURFACE_DIAGONAL_CENTERED_2, true, CELL_GCM_COMPMODE_C32_2X1, 0));
	Render::getColorSurfaceAsTexture(colorBuffer2xMSAA,tex_colorBuffer2xMSAA,CELL_GCM_TEXTURE_NR);

	// Reserve Depth Buffer MSAA 2x 1280 x 720 Z24
	MY_C(Render::createDepthSurface(depthBuffer2xMSAA, CELL_GCM_SURFACE_PITCH,
				RENDER_SURFACE_WIDTH, RENDER_SURFACE_HEIGHT,
				CELL_GCM_SURFACE_Z24S8, CELL_GCM_LOCATION_LOCAL,
				CELL_GCM_SURFACE_DIAGONAL_CENTERED_2, true,
				CELL_GCM_COMPMODE_Z32_SEPSTENCIL_DIAGONAL,2,true));
	Render::getDepthSurfaceAsTexture(depthBuffer2xMSAA,tex_depthBuffer2xMSAA,CELL_GCM_TEXTURE_NR);
}

void MyApp::initPreBuildContext(){
	initSetRenderTargetAndClearContext(&preBuildContext);
	initResolveMsaaContext(&preBuildContext);
	initSetVideoOutSurfaceContext(&preBuildContext);
	initClearVideoBufferContext(&preBuildContext);
}

void MyApp::initSetRenderTargetAndClearContext(CellGcmContextData* thisContext){
	uint32_t* begin = thisContext->current;
	callContextInfo[CALL_CONTEXT_INIT].init(Sys::Memory::getMappedMainMemoryHeap().AddressToOffset(begin));

	CellGcmSurface sf;
	Render::bindDepthColorSurface(&colorBuffer2xMSAA,&depthBuffer2xMSAA,0,0,sf);

	cellGcmSetPerfMonPushMarker(thisContext, "InitContextAndClear");
	cell::Gcm::Inline::cellGcmSetSurface(thisContext,&sf);
	cell::Gcm::Inline::cellGcmSetStencilMask(thisContext,0x00000000);
	cell::Gcm::Inline::cellGcmSetColorMask(thisContext, CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_A);
	cell::Gcm::Inline::cellGcmSetDepthMask(thisContext,CELL_GCM_TRUE);
	cell::Gcm::Inline::cellGcmSetClearColor(thisContext,0x000000CC);
	cell::Gcm::Inline::cellGcmSetClearDepthStencil(thisContext,0xFFFFFF00);
	cell::Gcm::Inline::cellGcmSetClearSurface(thisContext,
		CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B |
		CELL_GCM_CLEAR_A | CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_S );
	cell::Gcm::Inline::cellGcmSetDepthTestEnable(thisContext,CELL_GCM_TRUE);
	cell::Gcm::Inline::cellGcmSetDepthFunc(thisContext,CELL_GCM_LESS);
	cell::Gcm::Inline::cellGcmSetCullFace(thisContext,OPEN_GL_FRONT);
	cell::Gcm::Inline::cellGcmSetCullFaceEnable(thisContext,CELL_GCM_TRUE);
	cell::Gcm::Inline::cellGcmSetBlendEnable(thisContext,CELL_GCM_FALSE);
	cell::Gcm::Inline::cellGcmSetReportLocation(thisContext,CELL_GCM_LOCATION_MAIN);
	setViewportGL(thisContext, 0, 0, sf.width, sf.height, 0.0f, 1.0f, sf.height );
	cellGcmSetPerfMonPopMarker(thisContext);
	cell::Gcm::Inline::cellGcmSetReturnCommand(thisContext);
}

void MyApp::initResolveMsaaContext(CellGcmContextData* thisContext){
	uint32_t* begin = thisContext->current;
	callContextInfo[CALL_CONTEXT_RESOLVE_MSAA].init(Sys::Memory::getMappedMainMemoryHeap().AddressToOffset(begin));

	cellGcmSetPerfMonPushMarker(thisContext, "ResolveMSAA");
	cell::Gcm::Inline::cellGcmSetDepthTestEnable(thisContext,CELL_GCM_FALSE);
	cell::Gcm::Inline::cellGcmSetDepthMask(thisContext,CELL_GCM_FALSE);
	cell::Gcm::Inline::cellGcmSetColorMask(thisContext,CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B  );

	CGprogram vp = Render::ShaderManager::getVertexShader(Render::ShaderManager::VERTEX_SHADER_vpallquad);
	void* uCode;
	cellGcmCgGetUCode(vp,&uCode,NULL);
	cell::Gcm::Inline::cellGcmSetVertexProgram(thisContext, vp, uCode);

	CGprogram fp = Render::ShaderManager::getFragmentShader(Render::ShaderManager::FRAGMENT_SHADER_ds2xaccuview_shifted);
	uint32_t fpoffset = Render::ShaderManager::getFragmentShaderUcodeOffset(Render::ShaderManager::FRAGMENT_SHADER_ds2xaccuview_shifted);
	uint32_t fplocation = Render::ShaderManager::getFragmentShaderUcodeLocation(Render::ShaderManager::FRAGMENT_SHADER_ds2xaccuview_shifted);
	cellGcmSetFragmentProgramOffset(thisContext, fp, fpoffset, fplocation);
	cellGcmSetFragmentProgramControl(thisContext, fp, CELL_GCM_TRUE, 1, 0);

	cell::Gcm::Inline::cellGcmSetTexture(thisContext,MSAA_SAMPLER0,&tex_colorBuffer2xMSAA);
	cell::Gcm::Inline::cellGcmSetTextureControl(thisContext,MSAA_SAMPLER0, CELL_GCM_TRUE, 0, 15*256, CELL_GCM_TEXTURE_MAX_ANISO_1);
	cell::Gcm::Inline::cellGcmSetTextureAddress(thisContext,MSAA_SAMPLER0, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS,0);
	cell::Gcm::Inline::cellGcmSetTextureFilter(thisContext,MSAA_SAMPLER0, 0, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MAG, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);

	cell::Gcm::Inline::cellGcmSetTexture(thisContext, MSAA_SAMPLER1, &tex_colorBuffer2xMSAA);
	cell::Gcm::Inline::cellGcmSetTextureControl(thisContext, MSAA_SAMPLER1, CELL_GCM_TRUE, 0, 15 * 256, CELL_GCM_TEXTURE_MAX_ANISO_1 );
	cell::Gcm::Inline::cellGcmSetTextureAddress( thisContext, MSAA_SAMPLER1, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_LESS, 0);
	cell::Gcm::Inline::cellGcmSetTextureFilter(thisContext, MSAA_SAMPLER1, 0, CELL_GCM_TEXTURE_CONVOLUTION_MIN, CELL_GCM_TEXTURE_CONVOLUTION_MAG, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX_ALT);

	setViewport(thisContext, 0, 0, videoSurface.width, videoSurface.height, 0.0f, 1.0f);

	const Render::Mesh* all_quad = Render::MeshManager::getMesh(Render::MeshManager::MESH_ID_ALL_QUAD);
	cell::Gcm::Inline::cellGcmSetVertexDataArray(thisContext,VertexInfo::ATTRIBUTE_POSISION, 0, all_quad->position.stride, all_quad->position.size, all_quad->position.type , all_quad->position.location, all_quad->position.offset);
	for(int i=1; i < 16; i++){
		cell::Gcm::Inline::cellGcmSetVertexDataArrayFormat(thisContext, i, 0, 0, 0, CELL_GCM_VERTEX_F);
	}
	cell::Gcm::Inline::cellGcmSetDrawArrays(thisContext,CELL_GCM_PRIMITIVE_TRIANGLES,0,3);

	cellGcmSetPerfMonPopMarker(thisContext);
	cell::Gcm::Inline::cellGcmSetReturnCommand(thisContext);
}

void MyApp::initSetVideoOutSurfaceContext(CellGcmContextData* thisContext){
	const char* GcmMakerText[2] = {
		"Video Buffer Output 0",
		"Video Buffer Output 0"
	};
	for(uint32_t i=0; i < VIDEO_BUFFER_NUM; i++){
		uint32_t* begin = thisContext->current;
		callContextInfo[CALL_CONTEXT_SET_VIDEO_SURFACE0 + i].init(Sys::Memory::getMappedMainMemoryHeap().AddressToOffset(begin));
		videoSurface.colorOffset[0] = video_offset[i];

		cellGcmSetPerfMonPushMarker(thisContext, GcmMakerText[i]);

		cell::Gcm::Inline::cellGcmSetWaitFlip(thisContext); // 前のフリップの終了を待つ
		cell::Gcm::Inline::cellGcmSetSurface(thisContext,&videoSurface); // Videoサーフェースを設定する

		cellGcmSetPerfMonPopMarker(thisContext);
		cell::Gcm::Inline::cellGcmSetReturnCommand(thisContext); // Retrunする。
	}
}

void MyApp::initClearVideoBufferContext(CellGcmContextData* thisContext){
	//J 今回は使っていません
	uint32_t* begin = thisContext->current;
	callContextInfo[CALL_CONTEXT_CLEAR_VIDEO_BUFFER].init(Sys::Memory::getMappedMainMemoryHeap().AddressToOffset(begin));

	cellGcmSetPerfMonPushMarker(thisContext, "ClearVideoBuffer");

	cell::Gcm::Inline::cellGcmSetClearColor(thisContext,0x00000000);
	cell::Gcm::Inline::cellGcmSetColorMask(thisContext, CELL_GCM_COLOR_MASK_A | CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B); 
	cell::Gcm::Inline::cellGcmSetClearSurface(thisContext,
		CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);

	cellGcmSetPerfMonPopMarker(thisContext);
	cell::Gcm::Inline::cellGcmSetReturnCommand(thisContext); // Retrunする。
}

int32_t MyApp::CommandContextAssertCallback(CellGcmContextData *, uint32_t){
	MY_ASSERT(false);
	return 0;
}

void MyApp::initRender(){
	//J <コマンドバッファの初期化とputポインタの更新>
	uint32_t* jts = setJTS(gCellGcmCurrentContext); //J デフォルトコマンドバッファ上でJTSを作成する。
	cellGcmFlush(gCellGcmCurrentContext); //J putポインタを進めてJTSで止まるようにする。
	renderInitData.inout.lastJtsAddr = reinterpret_cast<uintptr_t>(jts); 	//J 最初のJTSのアドレスを設定します。

	//J <Constant Patch機能の設定>
	void* constant_patch_buffer = Sys::Memory::getLocalMemoryHeap().alloc(FRAGMENT_SHADER_PATCH_BUFFER_SIZE, KiB);
	fsConstantPatchContext.setupContext(constant_patch_buffer, FRAGMENT_SHADER_PATCH_BUFFER_SIZE, REPORT_INDEX_FS_CONSTANT_PATCH0);

	//J <SPU上でのAddressToOffsetの設定>
	spuAddressToOffsetInitData.setMap(SpuAddressToOffsetInitData::SPU_ADDRESS_TO_OFFSET_MAIN_MAP,
		Sys::Memory::getMappedMainMemoryHeap().getBegin(),
		Sys::Memory::getMappedMainMemoryHeap().getEnd(),
		Sys::Memory::getMappedMainMemoryHeap().getOffsetBase());
	spuAddressToOffsetInitData.setMap(SpuAddressToOffsetInitData::SPU_ADDRESS_TO_OFFSET_LOCAL_MAP,
		Sys::Memory::getLocalMemoryHeap().getBegin(),
		Sys::Memory::getLocalMemoryHeap().getEnd(),
		Sys::Memory::getLocalMemoryHeap().getOffsetBase());

	//J <Debug用にput/getレジスタのアドレスを取得>
	control = cellGcmGetControlRegister();
	status = cellGcmGetLabelAddress(LABEL_INDEX_COMMAND_STATUS);
}

void MyApp::quitRender(){
	//J putレジスタを取得する
	CellGcmControl* gcm_control = cellGcmGetControlRegister();
	//J JTSで止めているアドレスを取得する
	uint32_t* command = reinterpret_cast<uint32_t*>(renderInitData.inout.lastJtsAddr);
	//J Jump CommandでputレジスタへジャンプしRSXのコマンドフェッチを終了させる
	*command = CELL_GCM_METHOD_JUMP(gcm_control->put);
	//J デフォルトコマンドバッファ上に完全終了コマンドを作成する。See also RSX Tips 4.2.9
	volatile uint32_t* complete_end_status = cellGcmGetLabelAddress(LABEL_INDEX_COMPLETE_FINISH);
	*complete_end_status = (uint32_t)RSX_STATUS_FALSE; //J 完全終了でない状態とする
	cellGcmSetReferenceCommand(gCellGcmCurrentContext,(uint32_t)RSX_STATUS_FALSE); //J Referenceの値を期待値と異なる値にする
	cellGcmSetWriteBackEndLabel(gCellGcmCurrentContext, LABEL_INDEX_COMPLETE_FINISH, RSX_STATUS_TRUE);
	cellGcmSetWaitLabel(gCellGcmCurrentContext, LABEL_INDEX_COMPLETE_FINISH, RSX_STATUS_TRUE);
	cellGcmFinish(gCellGcmCurrentContext,RSX_STATUS_TRUE);
}

uint32_t* MyApp::setJTS(CellGcmContextData* thisContext){
	//J JumpToSelfを作成する
	uint32_t* jts_address = thisContext->current;
	uint32_t offset_of_jts = Sys::Memory::getMappedMainMemoryHeap().AddressToOffset(jts_address);
	cell::Gcm::Inline::cellGcmSetJumpCommand(thisContext,	offset_of_jts);
	return jts_address;
}

void MyApp::initObjects(){
	size_t mat_size = 0;
	Material* mat = Sys::Memory::getMainMemoryHeap().alloc<Material>();
	mat_size += sizeof(Material);
	const Render::Mesh* mesh = Render::MeshManager::getMesh(Render::MeshManager::MESH_ID_DUCK);
	mat->vertex_info.init();
	mat->vertex_info.SetFormat(VertexInfo::ATTRIBUTE_POSISION, 0, mesh->position.stride, mesh->position.size, mesh->position.type);
	mat->vertex_info.SetOffset(VertexInfo::ATTRIBUTE_POSISION, mesh->position.location,  mesh->position.offset);
	mat->vertex_info.SetFormat(VertexInfo::ATTRIBUTE_NORMAL, 0, mesh->normal.stride, mesh->normal.size, mesh->normal.type);
	mat->vertex_info.SetOffset(VertexInfo::ATTRIBUTE_NORMAL, mesh->normal.location,  mesh->normal.offset);
	mat->vertex_info.SetFormat(VertexInfo::ATTRIBUTE_TEXTURE_0, 0, mesh->texture.stride, mesh->texture.size, mesh->texture.type);
	mat->vertex_info.SetOffset(VertexInfo::ATTRIBUTE_TEXTURE_0, mesh->texture.location,  mesh->texture.offset);
	mat->vertex_info.SetIndex(mesh->num_vertex, mesh->index_offset, mesh->index_type, mesh->index_location, mesh->draw_mode);
	CGprogram fpBinary = Render::ShaderManager::getFragmentShader(Render::ShaderManager::FRAGMENT_SHADER_fpshader);
	mat->fragment_shader_info.binary_size = cellGcmCgGetTotalBinarySize(fpBinary);
	mat->fragment_shader_info.ea_ShaderBinary = reinterpret_cast<uintptr_t>(fpBinary);
	CGprogram vpBinary = Render::ShaderManager::getVertexShader(Render::ShaderManager::VERTEX_SHADER_vpshader);
	mat->vertex_shader_info.binary_size = cellGcmCgGetTotalBinarySize(vpBinary);
	mat->vertex_shader_info.ea_ShaderBinary = reinterpret_cast<uintptr_t>(vpBinary);

	mat_size += sizeof(TextureInfo);
	TextureInfo* texInfo = Sys::Memory::getMainMemoryHeap().alloc<TextureInfo>();
	const CellGcmTexture* tex = Render::TextureManager::getTexture(Render::TextureManager::TEXTURE_ID_DUCK);
	texInfo->SetIndex(FRAGMENT_SHADER_COLOR_SAMPLER_INDEX);
	texInfo->SetOffset(tex->offset);
	texInfo->SetFormat(tex->location, tex->format, tex->mipmap, tex->dimension, tex->cubemap, CELL_GCM_TEXTURE_BORDER_TEXTURE);
	texInfo->SetWrapAddress(CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_WRAP, CELL_GCM_TEXTURE_CLAMP_TO_EDGE,
		CELL_GCM_TEXTURE_UNSIGNED_REMAP_NORMAL, CELL_GCM_TEXTURE_ZFUNC_NEVER, 0, 0, CELL_GCM_TEXTURE_SIGNED_REMAP_NORMAL);
	texInfo->SetControl(0x0000,0x0000,CELL_GCM_TEXTURE_MAX_ANISO_1, CELL_GCM_FALSE);
	texInfo->SetRemap(CELL_GCM_TEXTURE_REMAP_ORDER_XXXY,
		CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP,
		CELL_GCM_TEXTURE_REMAP_FROM_B, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_A);
	texInfo->SetFilter(0x00, CELL_GCM_TEXTURE_NEAREST_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX,
		CELL_GCM_FALSE, CELL_GCM_FALSE, CELL_GCM_FALSE, CELL_GCM_FALSE);
	texInfo->SetBorderColor(0x00000000);
	texInfo->SetOptimization(0x8, CELL_GCM_TEXTURE_ISO_HIGH, CELL_GCM_TEXTURE_ANISO_HIGH);
	texInfo->SetSize(tex->height, tex->width, tex->depth, tex->pitch);
	mat->texture_info.setup(texInfo);
	mat->texture_number = 1;
	mat->size = mat_size;
	//mat->color = vec_float4{1.0f, 0.0f, 0.0f, 0.0f};
	vMaterial_ptr[MATERIAL_DUCK_A] = mat;

	srand(0x12345678);
	object_num = 0;

	float diffX = 1.0f;
	float diffY = 0.6f;
	float diffZ = 2.0f;
	for(int32_t z=0; z < OBJECT_Z_NUM; z++){
		for(int32_t y=0; y < OBJECT_Y_NUM; y++){
			for(int32_t x=0; x < OBJECT_X_NUM; x++){
				uint32_t i = x + y * OBJECT_X_NUM + z * OBJECT_X_NUM * OBJECT_Y_NUM;
				vObject[i].material = vMaterial_ptr[MATERIAL_DUCK_A];
				Work* work = vObject[i].getCurrentWork();
				Transform* trans = &work->transform;
				work->degree = rand() % 360 ;
				trans->scale = Vectormath::Aos::Vector3(2.4f,2.4f,2.4f);
				trans->translate = Vectormath::Aos::Vector3((diffX) * (x - OBJECT_X_NUM/2), (diffY) * (y - OBJECT_Y_NUM/2), (diffZ) * (z - OBJECT_Z_NUM/2));
				trans->rotate = Vectormath::Aos::Quat::rotationY(DEG2RAD(work->degree));
				object_num++;
				work->isOverwrite = (z == 0) ? 1 : 0;
				vec_float4 overwrite_light_color[3] =  {
						{1.0f,0.0f,1.0f,0.0f},
						{0.0f,0.0f,1.0f,0.0f},
						{0.0f,1.0f,0.0f,0.0f},
				};
				work->overwrite_light_color = overwrite_light_color[i%3];
			}
		}
	}

	camera.Set(Vectormath::Aos::Point3(0.0f,0.0f,-7.0f),
		Vectormath::Aos::Point3(0.0f,0.0f,0.0f),
		Vectormath::Aos::Vector3(0.0f,1.0f,0.0f));
	perspective.Set(DEG2RAD(45.0),16,9,0.1f,10.0f); 
	RenderEnv* current = getCurrentRenderEnv();
	Vectormath::Aos::Matrix4 proj = perspective.getGLProjectionMatrix();
	Vectormath::Aos::Matrix4 view = camera.getGLViewMatrix();
	current->View = view;
	current->ProjView = proj * view;
	Vectormath::Aos::Vector3 light_dir(-1.0f,0.7f,0.4f);
	Vectormath::Aos::normalize(light_dir);
	vec_float4 default_color =  {1.0f,1.0f,1.0f,0.0f}; 
	current->directional_light_dir = light_dir.get128();
	current->directional_light_color = default_color;
	current->directional_light_degree = 0;
}

void MyApp::update(){
	//J RenderEnvの更新
	{
		RenderEnv* current = getCurrentRenderEnv();
		RenderEnv* next = getNextRenderEnv();
		//J 前フレームをコピーする
		*next = *current;
		//J Lightの方向を回転させる
		next->directional_light_degree+=2;
		if(next->directional_light_degree >= 360) next->directional_light_degree -= 360;
		Vectormath::Aos::Vector3 light_dir(
			sinf(DEG2RAD(next->directional_light_degree)),
			1.0f,
			cosf(DEG2RAD(next->directional_light_degree)));
		Vectormath::Aos::normalize(light_dir);
		next->directional_light_dir = light_dir.get128();
	}
	//J オブジェクトWorkの更新
	for(uint32_t i=0; i < object_num; i++){
		Work* next		= vObject[i].getNextWork();
		Work* current	= vObject[i].getCurrentWork();
		//J 前フレームをコピーする
		*next = *current;
		//J 回転させる
		next->degree++;
		if(next->degree >= 360) next->degree -= 360;
		next->transform.rotate = Vectormath::Aos::Quat::rotationY(DEG2RAD(next->degree));
	}
}

void MyApp::pause(){
	//J PauseしているときはRenderEnvとオブジェクトWorkのコピーを行います。
	{
		RenderEnv* current = getCurrentRenderEnv();
		RenderEnv* next = getNextRenderEnv();
		*next = *current;
	}
	for(uint32_t i=0; i < object_num; i++){
		Work* next		= vObject[i].getNextWork();
		Work* current	= vObject[i].getCurrentWork();
		*next = *current;
	}
}

//J Flipコマンドを作成する
void MyApp::createFlipContext(CellGcmContextData* thisContext, uint32_t index){
	//J Note: Flipコマンドは毎フレーム作成しないといけません。
	//J また、内部でcallコマンドを使うので、callでつなぐことはできません。
	uint32_t* head = thisContext->current;
	cellGcmSetPerfMonPushMarker(thisContext, "FlipContext");
	for(int i=0; i < 16; i++){ // Disable Texture fetch for GPAD HUD.
		cell::Gcm::Inline::cellGcmSetTextureControl(thisContext, 0, CELL_GCM_FALSE, 0, 12 << 8, CELL_GCM_TEXTURE_MAX_ANISO_1);
	}
	MY_C(cellGcmSetFlip(thisContext, index));
	//J 次のフレーム向けに初期化とクリアを実行する。
	//J Note:ここに入れることでSPU起動とDrawPakets処理によるRSXのStavationを隠す。
	cell::Gcm::Inline::cellGcmSetCallCommand(thisContext,callContextInfo[CALL_CONTEXT_INIT].head_offset);
	cellGcmSetPerfMonPopMarker(thisContext);
	uint32_t* jts = setJTS(thisContext);
	jumpContextInfo[JUMP_CONTEXT_FLIP].init(head, jts);
}

void MyApp::createInitialPacketList(){
	//J 最初に処理するPacketList
	Sys::Memory::HeapBase& fheap = getCurrentFrameHeap();
	DrawPacketList& packetList = getPacketList();
	packetList.reset();
	DrawPacketNode* packet;

	//J Video出力用バッファを設定するDrawPacketを作成する
	packet = fheap.alloc<DrawPacketNode>();
	packet->createCallPacket(callContextInfo[CALL_CONTEXT_SET_VIDEO_SURFACE0 + getNextIndex()].head_offset);
	packetList.add(packet);

	//J Video出力バッファを単純にクリア
	packet = fheap.alloc<DrawPacketNode>();
	packet->createCallPacket(callContextInfo[CALL_CONTEXT_CLEAR_VIDEO_BUFFER].head_offset);
	packetList.add(packet);
}

void MyApp::createDrawPacketList(){
	//J 次のフレーム用のDrawPacketを用意する
	Sys::Memory::HeapBase& fheap = getNextFrameHeap();
	DrawPacketList& packetList = getPacketList();
	packetList.reset();
	DrawPacketNode* packet;

	for(uint32_t i=0; i < object_num; i++){
		packet = fheap.alloc<DrawPacketNode>();
		packet->createDrawPacket(vObject[i].material, vObject[i].getNextWork());
		//packet->setDebugBit(); //J デバッグビットでTagPacket処理時にSPUを停止させます。。
		packetList.add(packet);
	}

	//J Video出力用バッファを設定するDrawPacketを作成する
	packet = fheap.alloc<DrawPacketNode>();
	packet->createCallPacket(callContextInfo[CALL_CONTEXT_SET_VIDEO_SURFACE0 + getNextIndex()].head_offset);
	packetList.add(packet);

	//J MSAA resolve (Video出力バッファへMSAAを解決して書き込む)
	packet = fheap.alloc<DrawPacketNode>();
	packet->createCallPacket(callContextInfo[CALL_CONTEXT_RESOLVE_MSAA].head_offset);
	packetList.add(packet);
}

void MyApp::addRenderTask(){
	DrawPacketList& packetList = getPacketList();
	Sys::Memory::HeapBase& fheap = getCurrentFrameHeap();

	//J <Flip用のコマンドバッファを作成しDrawPacketListに追加する>
	//J Note: WaitForFlipの前に行うと、RSXがコマンドを処理している場合があるのでだめです。
	//J Flipは毎フレーム作成する必要があります。
	CellGcmContextData* flip_context = getCurrentFlipContext();
	resetContext(flip_context);
	createFlipContext(flip_context, getCurrentIndex());
	DrawPacketNode* packet = fheap.alloc<DrawPacketNode>();
	packet->createJumpPacket(jumpContextInfo[JUMP_CONTEXT_FLIP].head_address, jumpContextInfo[JUMP_CONTEXT_FLIP].last_address);
	packetList.add(packet);

	//J <SPU Renderへのパラメータ設定>
	//J FS Constant Patchの設定
	fsConstantPatchContext.initParameters(); // バッファを初期化する
	renderInitData.in.ea_FsConstantPatchContext = fsConstantPatchContext.getEa();
	//J コマンドバッファの設定
	resetContext(&mainContext);	// Main Contextの初期化
	renderInitData.in.spuCommandContextInitData.setContext(&mainContext);
	//J SpuAddressToOffsetInitData の設定
	renderInitData.in.ea_SpuAddressToOffsetInitData = spuAddressToOffsetInitData.getEa();
	//J PacketListを設定する
	renderInitData.inout.ea_current_packet = reinterpret_cast<uint32_t>(packetList.getHead());
	renderInitData.in.ea_RenderEnv = reinterpret_cast<uint32_t>(getCurrentRenderEnv());

	//J <Taskの生成>
	CellSpursTaskArgument arg;
	arg.u32[ARG_SPU_RENDER_EA_INIT_DATA] = renderInitData.getEa();
	arg.u32[ARG_SPU_RENDER_DEBUG_FLAG] = 0;
	MY_C(cellSpursCreateTask2WithBinInfo(taskset, &render_task_tid, 
		&_binary_task_spurs_task_render_elf_taskbininfo,
		&arg, render_task_context_buffer, "SPU_Render Task", NULL));
}

int MyApp::joinRenderTask(){
	int exitCode;
	MY_C(cellSpursJoinTask2(taskset, render_task_tid, &exitCode));
	MY_ASSERT(exitCode != EXIT_CODE_SPU_RENDER_FAILURE);
	return exitCode;
}

void MyApp::waitForFlip(){
	MY_C(sys_event_flag_wait(flip_event_flag, FLIP_EVENT_FLAG_FLIP_DONE,
		SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR_ALL, 0, SYS_NO_TIMEOUT));
}

void MyApp::init(){
	SysutilHandler::init();
	initBuffer();
	initRenderSurfaces();
	initVideoOut();
	initSpurs();
	Render::ShaderManager::init();
	Render::TextureManager::init();
	Render::MeshManager::init();
	initPreBuildContext();
	initRender();
	initObjects();
}

void MyApp::run(){
	createInitialPacketList();
	//J Main Rendering loop
	int count = 0;
	while(SysutilHandler::isRequestExit() == 0x0){
		//J SPU Render taskによるDrawPacketの処理を開始する
		addRenderTask();
		//J 次に使うFrame Heapを初期化する。
		getNextFrameHeap().reset();

		//J RenderEnv Objectを更新する。
		//J Note:SPURender/RSXが参照する物はダブルバッファ化する必要がある。
		if(SysutilHandler::isRequestPause()){pause();} // pauseの場合は更新しない
		else{ update(); }

		//J Spu Render Taskの終了を待つ
		//J Note: ここでPacketListは解放される
		joinRenderTask();
	
		//J 次のDrawPacketListを作成する
		createDrawPacketList();

		//J 次のIndexへ遷移する
		setNextBufferIndex();
		//J SysutilのCallbackをチェックする。
		SysutilHandler::update();
		//J Flipを待つ
		waitForFlip();
		count++;
		if(count == 2)
			return;
	}
}

void MyApp::quit(){
	quitRender();
	quitSpurs();
	quitVideoOut();
	SysutilHandler::quit();
}

uint32_t SysutilHandler::m_sysutil_request_register;
void SysutilHandler::sysutilCallback(uint64_t status, uint64_t param, void* userdata)
{
	(void) param;
	volatile uint32_t* requestCode = reinterpret_cast<uint32_t*>(userdata);

	switch(status) {
		case CELL_SYSUTIL_REQUEST_EXITGAME:
			*requestCode |= SYSUTIL_REQUEST_EXIT;
			break;
		case CELL_SYSUTIL_DRAWING_BEGIN:
			*requestCode |= SYSUTIL_DRAWING_ENABLE;
			break;
		case CELL_SYSUTIL_DRAWING_END:
			*requestCode &= (~SYSUTIL_DRAWING_ENABLE);
			break;
		case CELL_SYSUTIL_SYSTEM_MENU_OPEN:
			*requestCode |= SYSUTIL_REQUEST_PAUSE;
			break;
		case CELL_SYSUTIL_SYSTEM_MENU_CLOSE:
			*requestCode &= (~SYSUTIL_REQUEST_PAUSE);
			break;
		case CELL_SYSUTIL_BGMPLAYBACK_PLAY:
			*requestCode |= SYSUTIL_REQUEST_STOP_BGM;
			break;
		case CELL_SYSUTIL_BGMPLAYBACK_STOP:
			*requestCode &= (~SYSUTIL_REQUEST_STOP_BGM);
			break;
		case CELL_SYSUTIL_NP_INVITATION_SELECTED:
			// Not supported in this sample.
			break;
		default:
			;
	}
}

#include <sys/process.h>

SYS_PROCESS_PARAM(1001, 0x10000)
int main(int argc,char* argv[])
{
    (void)argc;(void)argv;
	//J MyAppのメンバをDMAする必要があるため、staticで確保します。
	//J Note: 非staticでstack上に確保すると、DMAに失敗します。
	static MyApp app; 

	app.init();
	app.run();
	app.quit();
	
	return CELL_OK;
}
