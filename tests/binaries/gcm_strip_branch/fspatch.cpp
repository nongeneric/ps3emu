/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#define __CELL_ASSERT__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// libgcm
#include <cell/gcm.h>
#include <Cg/cg.h>
#include <Cg/cgBinary.h>

// this project
#include "fspatch.h"

using namespace cell::Gcm;

namespace{
	inline uint32_t swap_16(const uint32_t v)
	{
		return (v>>16) | (v<<16);
	}
	inline uint32_t h_swap(const float f)
	{
		union f32_16{
			uint32_t ui;
			float f;
		} v;
		v.f = f;
		return swap_16(v.ui);
	}
}

void fspatchPatchParamDirect(CGprogram prog, CGparameter param, void *addr, float *value, uint32_t len)
{
	if(len > 4) return;

	// 書き換える値をHalfSwapする
	uint32_t HSWPD_VALUE[4];
	for(uint32_t i = 0; i < len; ++i){
		HSWPD_VALUE[i] = h_swap(value[i]);
	}
        
	// プログラム内の定数の参照回数を取得する
	uint32_t em_count = cellGcmCgGetEmbeddedConstantCount(prog, param);

	// プログラム内の定数を全て書き換える
	for(uint32_t i = 0; i < em_count; ++i) {
		uint32_t offset = cellGcmCgGetEmbeddedConstantOffset(prog, param, i);
		memcpy(reinterpret_cast<uint8_t*>(addr) + offset, (void*)HSWPD_VALUE, sizeof(uint32_t) * len);
	}
}
uint32_t fspatchGetParamIndex(CGprogram prog, CGparameter param)
{
    CgBinaryProgram *p = (CgBinaryProgram*) prog;
	CgBinaryParameter *ptr = (CgBinaryParameter*) param;

    if (p && ptr)
    {
		uint32_t start  = reinterpret_cast<uint32_t>((char*)p);
		uint32_t offset = reinterpret_cast<uint32_t>((char*)ptr);

		return (offset - start)/sizeof(CgBinaryParameter);
    }
    return 0xFFFFFFFF;
}
uint32_t fspatchGetNamedParamIndex(CGprogram prog, const char* name)
{
	CGparameter param = cellGcmCgGetNamedParameter(prog, name);
	CELL_GCMUTIL_CG_PARAMETER_CHECK_ASSERT( param );
	return fspatchGetParamIndex(prog, param);
}
uint8_t fspatchIsArrayParam(CGprogram prog, CGparameter param)
{
	switch (cellGcmCgGetParameterType(prog, param)) {
	case CG_FLOAT3x3:
	case CG_BOOL3x3:
	case CG_FLOAT4x3:
	case CG_BOOL4x3:
	case CG_FLOAT3x4:
	case CG_BOOL3x4:
	case CG_FLOAT4x4:
	case CG_BOOL4x4:
		return 1;
	default:
		return 0;
	}
}
uint16_t fspatchGetLength(CGprogram prog, CGparameter param)
{
	uint32_t length = 0;
	switch (cellGcmCgGetParameterType(prog, param)) {
	case CG_FLOAT:
	case CG_BOOL:
	case CG_FLOAT1:
	case CG_BOOL1:
		length = 1;
		break;
	case CG_FLOAT2:
	case CG_BOOL2:
		length = 2;
		break;
	case CG_FLOAT3:
	case CG_BOOL3:
		length = 3;
		break;
	case CG_FLOAT4:
	case CG_BOOL4:
		length = 4;
		break;
	case CG_FLOAT3x3:
	case CG_BOOL3x3:
	case CG_FLOAT3x4:
	case CG_BOOL3x4:
		length = 3;
		break;
	case CG_FLOAT4x4:
	case CG_BOOL4x4:
	case CG_FLOAT4x3:
	case CG_BOOL4x3:
		length = 4;
		break;
	default:
		length = 0;
	}
	return length;
}

uint32_t fspatchSetupFragmentPatchCgb(const CellCgbProgram *prog, Memory_t *patch_info, Memory_t *offset_list)
{
	// プログラム内の定数の数
	uint32_t param_count = cellCgbMapGetLength(prog);

	const uint32_t MAX_OFFSET_SIZE = 32;
	// 各定数の参照回数を合計
	uint32_t const_count = 0;
	for(uint32_t i = 0; i < param_count; ++i){
		uint32_t em_count = MAX_OFFSET_SIZE - 1;
		uint16_t em_offsets[MAX_OFFSET_SIZE];
		cellCgbMapGetFragmentUniformOffsets(prog, i, em_offsets, &em_count);
		em_offsets[em_count] = 0;

		const_count += em_count;
	}

	// patch_info
	uint32_t patch_info_size = sizeof(PatchParam_t) * param_count;
	if(cellGcmUtilAllocateUnmappedMain(patch_info_size, 128, patch_info) == false) return 0;

	// offset_list
	uint32_t offset_list_size = sizeof(uint32_t) * const_count;
	if(cellGcmUtilAllocateUnmappedMain(offset_list_size, 128, offset_list) == false)
	{
		cellGcmUtilFree(patch_info);
		return 0;
	}


	PatchParam_t *params = reinterpret_cast<PatchParam_t *>(patch_info->addr);
	uint32_t *offsets = reinterpret_cast<uint32_t *>(offset_list->addr);


	// 各定数の参照毎のオフセットを設定
	uint32_t start = 0;
	for(uint32_t i = 0; i < param_count; ++i){
		params[i].isEnable = 0;
		params[i].length = 0;

		// 定数の参照回数
		uint32_t em_count = MAX_OFFSET_SIZE - 1;
		uint16_t em_offsets[MAX_OFFSET_SIZE];
		cellCgbMapGetFragmentUniformOffsets(prog, i, em_offsets, &em_count);
		em_offsets[em_count] = 0;

		params[i].isEnable = 0;
		params[i].nArray = 0;
		params[i].length = 4;
		params[i].constCount = em_count;
		params[i].constStart = start;
		params[i].offsets = offsets;
		params[i].value[0] = params[i].value[1] = params[i].value[2] = params[i].value[3] = 0;

		// 参照毎のオフセット
		for(uint32_t j = 0; j < em_count; ++j) {
			offsets[start + j] = em_offsets[j];
		}

		start += em_count;
	}

	return param_count;
}

uint32_t fspatchSetupFragmentPatch(CGprogram prog, Memory_t *patch_info, Memory_t *offset_list)
{
	// プログラム内の定数の数
	uint32_t param_count = cellGcmCgGetCountParameter(prog);

	// 各定数の参照回数を合計
	uint32_t const_count = 0;
	for(uint32_t i = 0; i < param_count; ++i){
		CGparameter param = cellGcmCgGetIndexParameter(prog, i);
		uint32_t em_count = cellGcmCgGetEmbeddedConstantCount(prog, param);
		const_count += em_count;
	}

	// patch_info
	uint32_t patch_info_size = sizeof(PatchParam_t) * param_count;
	if(cellGcmUtilAllocateUnmappedMain(patch_info_size, 128, patch_info) == false) return 0;

	// offset_list
	uint32_t offset_list_size = sizeof(uint32_t) * const_count;
	if(cellGcmUtilAllocateUnmappedMain(offset_list_size, 128, offset_list) == false)
	{
		cellGcmUtilFree(patch_info);
		return 0;
	}


	PatchParam_t *params = reinterpret_cast<PatchParam_t *>(patch_info->addr);
	uint32_t *offsets = reinterpret_cast<uint32_t *>(offset_list->addr);


	// 各定数の参照毎のオフセットを設定
	uint32_t start = 0;
	for(uint32_t i = 0; i < param_count; ++i){
		params[i].isEnable = 0;
		params[i].length = 0;

		// 定数の参照回数
		CGparameter param = cellGcmCgGetIndexParameter(prog, i);
		uint32_t em_count = cellGcmCgGetEmbeddedConstantCount(prog, param);

		params[i].isEnable = 0;
		params[i].nArray = fspatchIsArrayParam(prog, param);
		params[i].length = fspatchGetLength(prog, param);
		params[i].constCount = em_count;
		params[i].constStart = start;
		params[i].offsets = offsets;
		params[i].value[0] = params[i].value[1] = params[i].value[2] = params[i].value[3] = 0;

		// 参照毎のオフセット
		for(uint32_t j = 0; j < em_count; ++j) {
			offsets[start + j] = cellGcmCgGetEmbeddedConstantOffset(prog, param, j);
		}

		start += em_count;
	}

	return param_count;
}
void fspatchSetPatchParam(PatchParam_t *param, const float* value)
{
	if(!param) return;
	if(!value) return;

	if(param->nArray){
		param->isEnable = 0;
		for (uint32_t i = 0; i < param->nArray; ++i){
			fspatchSetPatchParam(param + i, value + param->length * i);
		}
	}else{
		param->isEnable = 1;
		for (uint32_t i = 0; i < param->length; ++i){
			param->value[i] = h_swap(value[i]);
		}
	}
}
void fspatchSetPatchParamDirect(void *ucode, PatchParam_t *param, const float* value)
{
	if(!ucode) return;
	if(!param) return;
	if(!value) return;

	uint8_t *ptr = reinterpret_cast<uint8_t*>(ucode);
	if(param->nArray){
		for (uint32_t i = 0; i < param->nArray; ++i){
			fspatchSetPatchParamDirect(ptr, param + i, value + param->length * i);
		}
	}else{
		uint32_t swp_value[4];
		for (uint32_t i = 0; i < param->length; ++i){
			swp_value[i] = h_swap(value[i]);
		}
		for(uint32_t j = 0; j < param->constCount; ++j){
            uint32_t pos = param->constStart + j;
			memcpy(ptr + param->offsets[pos], swp_value, sizeof(uint32_t) * param->length);
		}
	}
}
void fspatchPatchParam(void *ucode, const PatchParam_t *params, uint32_t param_count)
{
	// パッチ情報を基に定数をpatching
	uint8_t *ptr = reinterpret_cast<uint8_t*>(ucode);
	for(uint32_t i = 0; i < param_count; ++i){
		if(params[i].nArray != 0) continue;
		if(params[i].isEnable != 1) continue;

		for(uint32_t j = 0; j < params[i].constCount; ++j){
            uint32_t pos = params[i].constStart + j;
			memcpy(ptr + params[i].offsets[pos], params[i].value, sizeof(uint32_t) * params[i].length);
		}
	}
}
