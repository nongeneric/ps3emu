#include <cell/dma.h>
#include "debug.h"
#include "spu_util.h"
#include "spu_render.h"
#include "spu_command_context.h"

void SpuCommandContext::setupFromInitData(const SpuCommandContextInitData& initData)
{
	m_ea_main_context = initData.ea_main_context;
	m_offset_of_main_context_begin = initData.offset_of_main_context_begin;
	cellDmaLargeGet(&m_main_context, m_ea_main_context, sizeof(m_main_context), TAG_FOR_INPUT_DATA, 0, 0);
	this->callback = SpuCommandContextCallback;
}

void SpuCommandContext::unloadMainContext(){
	cellDmaPut(&m_main_context, m_ea_main_context, sizeof(m_main_context), TAG_FOR_OUTPUT_DATA, 0, 0);
	m_ea_main_context = 0;
	m_offset_of_main_context_begin = 0;
}

inline void SpuCommandContext::flushInternal(uint32_t tag){
	size_t size = (uintptr_t)this->current - (uintptr_t)this->begin;
	if(m_main_context.current + size/sizeof(uint32_t) >= m_main_context.end){
		MY_ASSERT(false);//J メインメモリが不足しています。
	}
	if(size){
		//J 前のコマンドバッファ書き出しの終了を確認します。
		cellDmaWaitTagStatusAll(TAG_BIT(m_previous_dma_tag));
		//J コマンドバッファの書き出し
		cellDmaUnalignedPut(this->begin, (uint32_t)m_main_context.current, size, tag, 0,0);
		//J 書き出しtagを覚えておきます。
		m_previous_dma_tag = tag;
		//J main memory側のコマンドバッファ current ポインタの更新
		m_main_context.current += size / sizeof(uint32_t);
		//J LS上のコマンドバッファの更新
		m_buffer_index = (m_buffer_index + 1) % COMMAND_BUFFER_NUM;
		setupLsContext();
	}
}

void SpuCommandContext::flushContext(){
	flushInternal(TAG_FOR_COMMAND_BUFFER);
};

void SpuCommandContext::finishContext(){
	flushInternal(TAG_FOR_GPU_SYNC);
}

void SpuCommandContext::setupLsContext(){
	uint32_t* begin = m_command_buffer[m_buffer_index];
	uint32_t num_unaligned_16byte_commands = ((uintptr_t)m_main_context.current & 0xf) / sizeof(uint32_t);
	//J LSバッファのbegin ポインタとメインメモリのコマンドバッファのcurrentポインタのアライメントを一致させる
	this->begin = begin + num_unaligned_16byte_commands;
	this->current = this->begin;
	this->end = begin + NUM_GRAPHICS_COMMANDS;
}

int32_t SpuCommandContext::SpuCommandContextCallback(CellGcmContextData* current_context, uint32_t num_required_commands){
	 //J 確保したバッファしたバッファが要求数より少ない場合エラー
	MY_ASSERT(num_required_commands < SpuCommandContext::NUM_GRAPHICS_COMMANDS);
	SpuCommandContext* spu_context = reinterpret_cast<SpuCommandContext*>(current_context);
	spu_context->flushContext();
	return 0;
}
