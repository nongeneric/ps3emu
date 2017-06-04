#ifndef PPU_MY_APP_H
#define PPU_MY_APP_H

#include <cell/gcm.h>
#include <cell/spurs.h>
#include <sysutil/sysutil_sysparam.h>

#include "unit.h"
#include "rsx_resource.h"
#include "data.h"

#include "surface.h"
#include "memory.h"
#include "view.h"
#include "spu_render_common.h"

class SysutilHandler{
	enum {
		SYSUTIL_REQUEST_EXIT = 1,
		SYSUTIL_REQUEST_PAUSE = (1 << 1),
		SYSUTIL_REQUEST_STOP_BGM = (1 << 2),
		SYSUTIL_DRAWING_ENABLE = (1 << 3),
	};
	enum{
		EVENT_CALL_BACK_SLOT_0 = 0,
	};
public:
	inline static uint32_t getStatus(){return m_sysutil_request_register;}
	inline static uint32_t isRequestExit(){return m_sysutil_request_register & SYSUTIL_REQUEST_EXIT;}
	inline static uint32_t isRequestPause(){return m_sysutil_request_register & SYSUTIL_REQUEST_PAUSE;}
	inline static uint32_t isRequestStopBGM(){return m_sysutil_request_register & SYSUTIL_REQUEST_STOP_BGM;}
	inline static uint32_t isDrawing(){return m_sysutil_request_register & SYSUTIL_DRAWING_ENABLE;}
	inline static void update(){cellSysutilCheckCallback();}
	static void init(){
		m_sysutil_request_register = 0;
		MY_C(cellSysutilRegisterCallback(EVENT_CALL_BACK_SLOT_0, SysutilHandler::sysutilCallback, &m_sysutil_request_register));
	}
	static void quit(){
		MY_C(cellSysutilUnregisterCallback(EVENT_CALL_BACK_SLOT_0));

	}
private:
	static uint32_t m_sysutil_request_register;
	static void sysutilCallback(uint64_t status, uint64_t param, void* userdata);
};

class MyApp{
	struct PreBuildCallContextInfo{
		uint32_t  head_offset;
		inline void init(uint32_t offset){
			head_offset = offset;
		}		
	};

	struct PreBuildJumpContextInfo{
		spu_addr head_address;
		spu_addr last_address;
		inline void init(uint32_t* head_addr, uint32_t* last_addr){
			head_address = reinterpret_cast<spu_addr>(head_addr);
			last_address = reinterpret_cast<spu_addr>(last_addr);
		}
	}__attribute__ ((aligned(16)));

public:
	static const uint32_t RESERVE_FOR_GPAD				= 10 * MiB;
	static const uint32_t SEGMENT_SIZE					= 32 * KiB;
	static const uint32_t MINIMUM_CB_SIZE				= 64 * KiB; // Minimum
	static const uint32_t MAPPED_MEMORY_SIZE			= 16 * MiB;
	static const uint32_t MAPPED_MEMORY_FOR_REPORT_SIZE	= REPORT_INDEX_LAST * sizeof(CellGcmReportData);
	static const uint32_t MAIN_REPORT_OFFSET_BEGIN		= 0x0e000000;
	static const uint32_t LARGE_HEAP_SIZE				= 20 * MiB;
	static const uint32_t SMALL_HEAP_SIZE				= 10 * MiB;
	static const uint32_t PRE_BUILD_CONTEXT_SIZE		= 2 * MiB;
	static const uint32_t MAIN_CONTEXT_SIZE				= 6 * MiB;
	static const uint32_t VIDEO_BUFFER_NUM				= 2;
	static const uint32_t OPEN_GL_FRONT					= CELL_GCM_BACK;
	static const uint32_t OPEN_GL_BACK					= CELL_GCM_FRONT;
	static const uint32_t RENDER_SURFACE_WIDTH			= 1280;
	static const uint32_t RENDER_SURFACE_HEIGHT			= 720;
	static const uint32_t FRAME_BUFFER_SIZE				= 2 * MiB;
	static const uint32_t TEMPORARY_BUFFER_SIZE			= 4 * MiB;
	static const uint32_t FRAGMENT_SHADER_PATCH_BUFFER_SIZE	= 48 * KiB;
	static const uint32_t FLIP_CONTEXT_SIZE				= 1 * KiB;
	static const int32_t OBJECT_X_NUM					= 7;
	static const int32_t OBJECT_Y_NUM					= 6;
	static const int32_t OBJECT_Z_NUM					= 3;
	static const int32_t OBJECT_MAX					= OBJECT_X_NUM * OBJECT_Y_NUM * OBJECT_Z_NUM;

	MyApp(){};
	void init();
	void run();
	void quit();

private:
	enum{
		CALL_CONTEXT_INIT = 0, // include Setup Render Surface, Clear Surface
		CALL_CONTEXT_RESOLVE_MSAA,
		CALL_CONTEXT_CLEAR_VIDEO_BUFFER,
		CALL_CONTEXT_SET_VIDEO_SURFACE0, 
		CALL_CONTEXT_SET_VIDEO_SURFACE1,
		CALL_CONTEXT_END,
	};

	enum{
		JUMP_CONTEXT_FLIP = 0,
		JUMP_CONTEXT_END,
	};

	// Surface
	Render::ColorSurface colorBuffer2xMSAA;
	CellGcmTexture tex_colorBuffer2xMSAA;
	Render::DepthSurface depthBuffer2xMSAA;
	CellGcmTexture tex_depthBuffer2xMSAA;
	void initRenderSurfaces();

	Sys::Memory::HeapBase frameHeap[VIDEO_BUFFER_NUM];
	Sys::Memory::HeapBase& getCurrentFrameHeap(){ return frameHeap[getCurrentIndex()]; }
	Sys::Memory::HeapBase& getNextFrameHeap(){ return frameHeap[getNextIndex()]; }

	// VideoOut
	static uint8_t buffer_index;
	inline static uint8_t getCurrentIndex(){return buffer_index;}
	inline static uint8_t getNextIndex(){return (buffer_index + 1) % VIDEO_BUFFER_NUM;}
	inline static void setNextBufferIndex(){buffer_index = getNextIndex();}
	static sys_event_flag_t flip_event_flag;
	static void FlipHandler(const uint32_t head);
	enum{
		FLIP_EVENT_FLAG_FLIP_DONE = (1 << 0)
	};
	CellGcmSurface videoSurface;
	CellVideoOutResolution		resolution;
	uint32_t video_offset[VIDEO_BUFFER_NUM];
	void initVideoOut();
	void quitVideoOut();
	inline uint16_t getVideoWidth(){return resolution.width;}
	inline uint16_t getVideoHeight(){return resolution.height;}
	inline uint32_t getVideoOffset(int index){return video_offset[index];}

	// Spurs
	static const int SPURS_NUM_SPU = 5;
	static const int SPURS_SPU_THREAD_GROUP_PRIORITY = 100;
	static const int SPURS_HANDLER_THREAD_PRIORITY = 2;
	static const bool SPURS_RELEASE_SPU_WHEN_IDLE = false;
	static const int SPURS_CONTEXT_BUFFER_ALIGN = 128;
	CellSpurs2* spurs2;
	CellSpursTaskset2* taskset;
	CellSpursTaskId render_task_tid;
	void* render_task_context_buffer;
	void initSpurs();
	void quitSpurs();
	void addRenderTask();
	int joinRenderTask();

	// Memory 
	void initBuffer();

	// Shader
	void initShader();

	// Graphics Command Generation
	DrawPacketList packetList;
	inline DrawPacketList& getPacketList(){ return packetList; }
	PreBuildCallContextInfo callContextInfo[CALL_CONTEXT_END];
	PreBuildJumpContextInfo jumpContextInfo[JUMP_CONTEXT_END];
	CellGcmReportData* report_begin;
	CellGcmContextData mainContext __attribute__ ((aligned(16)));
	CellGcmContextData preBuildContext;
	CellGcmContextData flipContext[VIDEO_BUFFER_NUM];
	CellGcmContextData* getCurrentFlipContext(){ return &flipContext[getCurrentIndex()]; }
	CellGcmContextData* getNextFlipContext(){ return &flipContext[getNextIndex()]; }
	CellGcmControl* control;
	uint32_t*		status;

	void initPreBuildContext();
	void initSetRenderTargetAndClearContext(CellGcmContextData* thisContext);
	void initResolveMsaaContext(CellGcmContextData* thisContext);
	void initSetVideoOutSurfaceContext(CellGcmContextData* thisContext);
	void initClearVideoBufferContext(CellGcmContextData* thisContext);
	void createFlipContext(CellGcmContextData* thisContext, uint32_t index);
	inline void resetContext(CellGcmContextData* context){
		context->current = context->begin;
	}
	static int32_t CommandContextAssertCallback(CellGcmContextData *, uint32_t);

	// Rendering flow
	enum{
		MATERIAL_DUCK_A,
		MATERIAL_DUCK_B,
		MATERIAL_DUCK_C,
		MATERIAL_DUCK_D,
		MATERIAL_DUCK_E,
		MATERIAL_DUCK_F,
		MATERIAL_DUCK_G,
		MATERIAL_MAX_NUM,
	};

	struct Object{
		Material* material;
		Work work[VIDEO_BUFFER_NUM];
		inline Work* getCurrentWork(){ return &work[getCurrentIndex()];}
		inline Work* getNextWork(){ return &work[getNextIndex()];}
	};

	SpuRenderInitData renderInitData;
	Camera camera;
	Perspective perspective;
	RenderEnv renderEnv[VIDEO_BUFFER_NUM];
	inline RenderEnv* getCurrentRenderEnv(){ return &renderEnv[getCurrentIndex()];}
	inline RenderEnv* getNextRenderEnv(){ return &renderEnv[getNextIndex()];}

	FsConstantPatchContext fsConstantPatchContext;
	SpuAddressToOffsetInitData spuAddressToOffsetInitData;
	Material* vMaterial_ptr[MATERIAL_MAX_NUM];
	Object vObject[OBJECT_MAX];
	uint32_t object_num;

	void initRender();
	void quitRender();
	void initObjects();
	void update();
	void pause();
	void procFrame();
	void createInitialPacketList();
	void createDrawPacketList();
	void waitForFlip();
	static uint32_t* setJTS(CellGcmContextData* thisContext);
};

#endif // PPU_MY_APP_H
