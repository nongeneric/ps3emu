#include <cell/spurs.h>
#include "spu_render.h"
#include <spu_printf.h>
#include "fs_constant_patch.h"

FsConstantPatch* FsConstantPatch::m_instance = NULL;

uint32_t FsConstantPatch::sendFragmentProgramInternal(CellGcmContextData* context, CGprogram fragmentShader){
	CgBinaryProgram* fs = reinterpret_cast<CgBinaryProgram*>(fragmentShader);
	uint32_t code_size = fs->ucodeSize;
	uint32_t required_buffer_size = calcBufferSizeWithOverfetch(code_size);

	//J Local MemoryにBufferを確保する。
	uint32_t ea_reserved_buffer = (getEaSegmentCurrent() + CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET - 1) & ~( CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET - 1);
	if(ea_reserved_buffer + required_buffer_size >= getEaSegmentEnd()){ //J セグメント内のバッファが不足した場合、次のSegmentへ移動する
		switchToNextSegment(context);
		ea_reserved_buffer = (getEaSegmentCurrent() + CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET - 1) & ~( CELL_GCM_FRAGMENT_UCODE_LOCAL_ALIGN_OFFSET - 1);
	}
	setEaSegmentCurrent(ea_reserved_buffer + required_buffer_size);	//J ポインタを必要なバッファ分進める

	//J ucodeをLocal Memoryに転送する。
	//J See also: RSX Access Ordering 1.3.4.4. SPU からのJTS 解除による制御
	static vec_uint4 dirtyBuffer;
	uint32_t* ucode = reinterpret_cast<uint32_t*>((uintptr_t)fs + fs->ucode);
	cellDmaLargePut(ucode, ea_reserved_buffer, code_size, TAG_FOR_GPU_SYNC,0,0);
	char buf[500] = {0};
	//spu_printf("sendFragmentProgramInternal 1 (%x, %x, %x): %08x %08x %08x %08x\n", ucode, ea_reserved_buffer, code_size,
	//	ucode[0], ucode[1], ucode[2], ucode[3]);
	cellDmaSmallGetf(&dirtyBuffer, ea_reserved_buffer, 4, TAG_FOR_GPU_SYNC,0,0);

	//J offset値を取得する
	uint32_t offset = m_instance->getOffset(ea_reserved_buffer);
	return offset;
}

void FsConstantPatch::switchToNextSegment(CellGcmContextData* context){

	volatile CellGcmReportData* ls_report_data_read_buffer = reinterpret_cast<CellGcmReportData*>(CELL_SPURS_LOCK_LINE);

	//J Atomicを利用してReport領域のzeroメンバを非ゼロ(SHADER_PRODUCED)にマークする
	//J ここでは普通にDMAで更新してもよい。
	do{
		cellDmaGetllar(ls_report_data_read_buffer, getEaMainMemoryReportStart(), 0, 0);
		cellDmaWaitAtomicStatus();

		ls_report_data_read_buffer[getSegmentIndex()].zero = SHADER_PRODUCED;
		cellDmaPutllc(ls_report_data_read_buffer, getEaMainMemoryReportStart(), 0, 0);
	}while(cellDmaWaitAtomicStatus());

	//J  RSXによって、RSX_CONSUMEDにマークさせるためにReportコマンドを設定する。
	cellGcmSetReportInline(context, CELL_GCM_ZPASS_PIXEL_CNT, getSegmentIndex() + getReportIndexBegin() );

	uint32_t nextSegmentIdx = (getSegmentIndex() + 1) % SEGMENT_NUM;
#if 0 //J Yieldを伴わないポーリングを行う場合
	//J Atomic更新時に読み込んだデータを利用して、次のバッファのステータスを確認する。
	if(unlikely(ls_report_data_read_buffer[nextSegmentIdx].zero != RSX_CONSUMED)){
		spu_write_event_mask(MFC_LLR_LOST_EVENT); // LockLine Reservation 消失イベントを有効化する。
		do{
			spu_write_event_ack(MFC_LLR_LOST_EVENT); // LockLine Reservation 消失イベントをAcknowledgeしてリセットする。
			//J LockLine Reservationとともに再度読み込みを行う
			cellDmaGetllar(ls_report_data_read_buffer, getEaMainMemoryReportStart(), 0, 0);
			cellDmaWaitAtomicStatus(); // Lock Line Resevationを確保する。
			if(ls_report_data_read_buffer[nextSegmentIdx].zero == RSX_CONSUMED)
				break;//J 更新されていれば、Breakする。
			spu_read_event_status(); //J LockLine Reservation 消失イベントを待つ
		}while(1); 
		spu_write_event_mask(0); //J LockLine Reservation 消失イベントを無効化する。
	}
#else //J Yieldを伴うポーリングを行う場合
	if(unlikely(ls_report_data_read_buffer[nextSegmentIdx].zero != RSX_CONSUMED)){
		do{
			spu_write_event_mask(MFC_LLR_LOST_EVENT); //J LockLine Reservation 消失イベントを有効化する。
			spu_write_event_ack(MFC_LLR_LOST_EVENT); //J LockLine Reservation 消失イベントをAcknowledgeしてリセットする。
			//J LockLine Reservationとともに再度読み込みを行う
			cellDmaGetllar(ls_report_data_read_buffer, getEaMainMemoryReportStart(), 0, 0);
			cellDmaWaitAtomicStatus(); //J Lock Line Resevationを確保する。
			if(ls_report_data_read_buffer[nextSegmentIdx].zero == RSX_CONSUMED)
				break;//J 更新されていれば、Breakする。

			do{
				//J Eventのノンブロッキング関数spu_stat_event_status()による
				//J LockLine Reservation 消失イベント待ち
				if(spu_stat_event_status()) break;
				
				//J 最後のTaskPoll()から一定時間経過した場合は、TaskPollを実行し、必要な場合はYieldする
				uint32_t current = spu_read_decrementer();
				uint32_t delta = (SpuRender::decremneter > current) ? SpuRender::decremneter - current : SpuRender::decremneter + (0xffffffff - current);
				if(delta > SpuRender::kTaskPollCycleInDecrementerCount){
					MY_LSGUARD_CHECK();
					if(cellSpursTaskPoll2()){
						cellSpursYield(); // 
						//J Yieldから戻ってきたとき、decrementerの値が大きく変わっている可能性があるため再度読み込む
						SpuRender::decremneter = spu_read_decrementer();
					}
					else{
						SpuRender::decremneter = current;
					}
					MY_LSGUARD_REHASH();
					//J これらの関数内で、LockLine Resevation 消失イベントが
					//J 無効化される可能性があるので、先頭で再度有効化する。
					break;
				}
			}while(1);
		}while(1);
	}
#endif
	setSegment(nextSegmentIdx);//J 利用するセグメントを更新する

	//J バッファをラップラウンドする時に、Texture Cacheをクリアする。
	if (unlikely(nextSegmentIdx == 0)){
		cellGcmSetWaitForIdleInline(context); //J WaitForIdleで選考するdrawコマンドを完全に終了させる
		cellGcmSetInvalidateTextureCacheInline(context,CELL_GCM_INVALIDATE_TEXTURE);
	}
}

inline vec_uint4 swap_hw_in_word(vec_float4 v)
{
	vec_uint4 data = (vec_uint4)v;
	return spu_sl(data, 16) | spu_rlmask(data, -16);
}

void patchFragmentProgramParameter(const CGprogram prog, const CGparameter param, const vec_float4* value){
	const CgBinaryProgram* fs = reinterpret_cast<const CgBinaryProgram*>(prog);
	const CgBinaryParameter* p = reinterpret_cast<const CgBinaryParameter*>(param);
	vec_uint4* ucode = reinterpret_cast<vec_uint4*>(reinterpret_cast<uintptr_t>(fs) + fs->ucode);
	uint32_t count_max = 1;
	switch (p->type){
			case CG_FLOAT:
			case CG_BOOL:
			case CG_FLOAT1:
			case CG_BOOL1:
			case CG_FLOAT2:
			case CG_BOOL2:
			case CG_FLOAT3:
			case CG_BOOL3:
			case CG_FLOAT4:
			case CG_BOOL4:
				count_max = 1;
				break;
			case CG_FLOAT3x3: // Assume vector4 x 3
			case CG_BOOL3x3:
			case CG_FLOAT3x4:
			case CG_BOOL3x4:
				p++;
				count_max = 3;
				break;
			case CG_FLOAT4x3: // Assume vector4 x 4
			case CG_BOOL4x3:
			case CG_FLOAT4x4:
			case CG_BOOL4x4:
				p++;
				count_max = 4;
				break;
			default:
				MY_ASSERT(false);
				break;
	}

	for (uint32_t cnt = 0; cnt < count_max; cnt++, p++, value++) {
		if (p->embeddedConst)
		{
			// set embedded constants
			CgBinaryEmbeddedConstant *ec = (CgBinaryEmbeddedConstant*) 
				(reinterpret_cast<uintptr_t>(fs) + p->embeddedConst);
			vec_uint4 val;
			// hw bug workaround, swap 16bit value
			val = swap_hw_in_word(*value);
			//J countはほぼ1〜3なので3つアンロール
			uint32_t count = ec->ucodeCount;
			const uint32_t* index = ec->ucodeOffset;
			unsigned index0 = index[0]/16;
			unsigned index1 = index[1]/16;
			unsigned index2 = index[2]/16;
			index1 = 1 < count ? index1 : index0;
			index2 = 2 < count ? index2 : index0;
			ucode[index0] = val;
			ucode[index1] = val;
			ucode[index2] = val;
			//J 残りは条件分岐で飛ばしてからループ処理
			if UNLIKELY(3 < count) {
				for (unsigned i = 3; i < count; ++i) {
					ucode[index[i]] = val;
				}
			}
		}
	}
}