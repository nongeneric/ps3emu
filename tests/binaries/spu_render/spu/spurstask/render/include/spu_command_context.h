#ifndef SPURS_TASK_RENDER_SPU_COMMAND_CONTEXT_H
#define SPURS_TASK_RENDER_SPU_COMMAND_CONTEXT_H

#include <cell/gcm_spu.h>
#include <cell/gcm/gcm_method_data.h>
#include "spu_address_to_offset.h"
#include "spu_render_tag.h"

class SpuCommandContext : public cell::Gcm::Inline::CellGcmContext{
public:
	enum{
		COMMAND_BUFFER_NUM = 2,
		NUM_GRAPHICS_COMMANDS = 256,
	};
	SpuCommandContext(){};
	~SpuCommandContext(){};
	void setupFromInitData(const SpuCommandContextInitData& initData);
	void unloadMainContext();
	inline void initContext(){
		m_buffer_index = 0;
		setupLsContext();
	}
	void flushContext();
	void finishContext();
	//J LSコマンドバッファ上でのcurrentが指し示すMainMemory上でOffset値を取得します。
	inline uint32_t getCurrentMainContextAddress(){
		uint32_t diff_ls_command_buffer = (uintptr_t)current - (uintptr_t)begin;
		return (uintptr_t)m_main_context.current + diff_ls_command_buffer;
	}
	inline uint32_t getCurrentCommandBufferOffset(uint32_t ea){
		uint32_t offset = (ea - (uintptr_t)m_main_context.begin) + m_offset_of_main_context_begin;
		return offset;
	}
	inline uint32_t getMainContextCurrentEa(){
		return reinterpret_cast<uint32_t>(m_main_context.current);
	}

private:
	uint32_t m_buffer_index;
	uint32_t m_offset_of_main_context_begin;
	uint32_t m_ea_main_context;
	uint32_t m_previous_dma_tag;
	CellGcmContextData m_main_context __attribute__ ((aligned(16))); // Command buffer of main memory.
	uint32_t m_command_buffer[COMMAND_BUFFER_NUM][NUM_GRAPHICS_COMMANDS] __attribute__ ((aligned(128)));
	void setupLsContext();
	inline void flushInternal(uint32_t tag);
	static int32_t SpuCommandContextCallback(CellGcmContextData* current_context, uint32_t num_required_commands);

public: //J グラフィックスコマンド生成
	inline uint32_t GetVertexProgramConstantVector4Size(){ return 8;};
	inline void SetVertexProgramConstantVector4(unsigned index, const float *data, unsigned update)
	{
		MY_ASSERT(!update || index <= 467 );
		MY_ASSERT(((uintptr_t)this & 15) == 0);

		vec_uint4* v_cmdptr = (vec_uint4*)this->current;
		this->current += update ? 8: 0;

		vec_uint4 header = (vec_uint4){
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_HEADER_VERTEX_CONSTANT_LOAD(5),
			index
		};
		vec_uint4 data0 = ((const vec_uint4*)data)[0];
		v_cmdptr[0] = header;
		v_cmdptr[1] = data0;
	}

	inline uint32_t GetVertexProgramConstantsMatrix4x4AosSize(){ return 20;};
	inline void SetVertexProgramConstantsMatrix4x4Aos(unsigned index, const float *data, unsigned update)
	{
		MY_ASSERT(!update || index <= 467 );
		MY_ASSERT(((uintptr_t) this->current & 15) == 0);

		vec_uint4* v_cmdptr = (vec_uint4*) this->current;
		this->current += update ? 20 : 0;

		vec_uint4 header = (vec_uint4){
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_NOP,
			CELL_GCM_METHOD_HEADER_VERTEX_CONSTANT_LOAD(17),
			index
		};

		vec_uint4 data0 = ((const vec_uint4*)data)[0];
		vec_uint4 data1 = ((const vec_uint4*)data)[1];
		vec_uint4 data2 = ((const vec_uint4*)data)[2];
		vec_uint4 data3 = ((const vec_uint4*)data)[3];

		static const vec_uchar16 shuf_xayb = {
			0x00,0x01,0x02,0x03,
			0x10,0x11,0x12,0x13,
			0x04,0x05,0x06,0x07,
			0x14,0x15,0x16,0x17
		};
		static const vec_uchar16 shuf_zcwd = {
			0x08,0x09,0x0a,0x0b,
			0x18,0x19,0x1a,0x1b,
			0x0c,0x0d,0x0e,0x0f,
			0x1c,0x1d,0x1e,0x1f
		};
		vec_uint4 tmp0 = spu_shuffle(data0, data2, shuf_xayb);
		vec_uint4 tmp1 = spu_shuffle(data1, data3, shuf_xayb);
		vec_uint4 tmp2 = spu_shuffle(data0, data2, shuf_zcwd);
		vec_uint4 tmp3 = spu_shuffle(data1, data3, shuf_zcwd);

		v_cmdptr[0] = header;
		v_cmdptr[1] = spu_shuffle(tmp0, tmp1, shuf_xayb);
		v_cmdptr[2] = spu_shuffle(tmp0, tmp1, shuf_zcwd);
		v_cmdptr[3] = spu_shuffle(tmp2, tmp3, shuf_xayb);
		v_cmdptr[4] = spu_shuffle(tmp2, tmp3, shuf_zcwd);
	}

	void SetVertexProgram(const CGprogram prog)
	{
		MY_ASSERT(prog);
		const CgBinaryParameter* param;

        const CgBinaryProgram* vs = reinterpret_cast<const CgBinaryProgram*>(prog);
        // check binary format revision
        MY_ASSERT(vs->binaryFormatRevision == CG_BINARY_FORMAT_REVISION);
		CgBinaryVertexProgram* binaryVertexProgram = reinterpret_cast<CgBinaryVertexProgram*> ((char*)vs + vs->program);

        uint32_t instCount = binaryVertexProgram->instructionCount;
        uint32_t instIndex = binaryVertexProgram->instructionSlot;

        // check program size
        MY_ASSERT(instCount * 16 == vs->ucodeSize);
        MY_ASSERT((instIndex + instCount) <= CELL_GCM_VTXPRG_MAX_INST);

		unsigned constCount = vs->parameterCount;
		unsigned reserve = (2 + ((instCount + 7) >> 3) * 9 + constCount * 2) * 4;
		this->ReserveMethodSize(reserve);
		MY_ASSERT(((uintptr_t) this->current & 15) == 0);
		vec_uint4* v_cmdptr = (vec_uint4*)this->current;

		{
			vec_uint4 header0 = (vec_uint4){
				CELL_GCM_METHOD_HEADER_VERTEX_PROGRAM_LOAD(2),
				instIndex,
				instIndex,
				CELL_GCM_METHOD_NOP
			};
			vec_uint4 header1 = (vec_uint4){
				CELL_GCM_METHOD_HEADER_VERTEX_ATTRIB_INPUT_MASK(),
					binaryVertexProgram->attributeInputMask,
				CELL_GCM_METHOD_HEADER_VERTEX_TIMEOUT(1),
				CELL_GCM_METHOD_DATA_VERTEX_TIMEOUT(0xFFFF,	(binaryVertexProgram->registerCount <= 32) ? 32 : 48)
			};
			v_cmdptr[0] = header0;
			v_cmdptr[1] = header1;
			v_cmdptr += 2;
		}

		// upload microcodes
		const vec_uint4* code = reinterpret_cast<vec_uint4*>((char*)vs + vs->ucode);
		while (instCount) {
			unsigned i = instCount < 8 ? instCount : 8;
			vec_uint4 header = (vec_uint4){
				CELL_GCM_METHOD_NOP,
				CELL_GCM_METHOD_NOP,
				CELL_GCM_METHOD_NOP,
				CELL_GCM_METHOD_HEADER_VERTEX_PROGRAM(i * 4)
			};
			instCount -= i;
			vec_uint4 code0 = code[0];
			vec_uint4 code1 = code[1];
			vec_uint4 code2 = code[2];
			vec_uint4 code3 = code[3];
			vec_uint4 code4 = code[4];
			vec_uint4 code5 = code[5];
			vec_uint4 code6 = code[6];
			vec_uint4 code7 = code[7];
			code += i;

			v_cmdptr[0] = header;
			v_cmdptr[1] = code0;
			v_cmdptr[2] = code1;
			v_cmdptr[3] = code2;
			v_cmdptr[4] = code3;
			v_cmdptr[5] = code4;
			v_cmdptr[6] = code5;
			v_cmdptr[7] = code6;
			v_cmdptr[8] = code7;
			v_cmdptr += i + 1;
		}
        param = (const CgBinaryParameter *)((const char *)vs + vs->parameterArray);
        for(uint32_t paramCount = vs->parameterCount; paramCount-- > 0;)
        {
				vec_uint4 header = (vec_uint4){
					CELL_GCM_METHOD_NOP,
					CELL_GCM_METHOD_NOP,
					CELL_GCM_METHOD_HEADER_VERTEX_CONSTANT_LOAD(5),
					param->resIndex};

				v_cmdptr[0] = header;
				v_cmdptr[1] = *reinterpret_cast<const vec_uint4*>((const char *)vs + param->defaultValue);

				//J 条件によってポインタを進めるか進めないかを切り替える
				v_cmdptr += (param->defaultValue && ((param->var == CG_CONSTANT) || (param->var == CG_UNIFORM))) 
					? 2 : 0;
                ++param;
        }
		current = reinterpret_cast<uint32_t*>(v_cmdptr);
	}

	inline void Align()
	{
		this->ReserveMethodSize(4);
		// current qword
		vec_uint4* v_cmdptr = (vec_uint4*)((uintptr_t) current & ~15);

		// fill CELL_GCM_METHOD_NOP
		uintptr_t alignment = (uintptr_t) current & 15;
		vec_uint4 mask = (vec_uint4)spu_maskb(~0xffffu >> alignment);
		*v_cmdptr = spu_and(*v_cmdptr, mask);
		// ceil qword
		current = reinterpret_cast<uint32_t*>((uintptr_t)(current + 3) & ~15);
	}

	//J JTSをLSコマンドバッファ上に作成し、そのEAを返す
	inline uint32_t SetJumpToSelf(){
		this->ReserveMethodSize(1);
		cell::Gcm::UnsafeInline::CellGcmContext* context = reinterpret_cast<cell::Gcm::UnsafeInline::CellGcmContext*>(this);
		uint32_t ea_JTS = getCurrentMainContextAddress();
		context->SetJumpCommand(getCurrentCommandBufferOffset(ea_JTS));
		return ea_JTS;
	}
	
	inline void SetVertexDataArray(uint32_t index, uint32_t gcm_method_data_format, uint32_t gcm_method_data_offset, uint32_t update){
		vec_uint4 data = (vec_uint4){
			CELL_GCM_METHOD_HEADER_VERTEX_DATA_ARRAY_FORMAT(index,1),
			gcm_method_data_format,
			CELL_GCM_METHOD_HEADER_VERTEX_DATA_ARRAY_OFFSET(index,1),
			gcm_method_data_offset
		};
		MY_ASSERT(((uintptr_t) this->current & 15) == 0);
		
		vec_uint4* v_cmdptr = (vec_uint4*) this->current;
		this->current += update ? 4 : 0;
		v_cmdptr[0] = data;
	}
};

//J JumpToSelf解除コマンド生成
inline void releaseJTStoJTN(uint32_t ea_JTS, uint32_t jumpAddress){
	static uint32_t buffer[4] __attribute__ ((aligned(16)));
	uint32_t index = (ea_JTS & 0xf) / sizeof(uint32_t);
	//J JumpToNext コマンドを作る
	buffer[index] = CELL_GCM_METHOD_JUMP(SpuAddressToOffset::getOffset(jumpAddress));
	cellDmaUnalignedPutf(&buffer[index],ea_JTS,sizeof(uint32_t),TAG_FOR_GPU_SYNC,0,0);
}

#endif // SPURS_TASK_RENDER_SPU_COMMAND_CONTEXT_H