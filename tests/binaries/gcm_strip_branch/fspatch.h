/*  SCE CONFIDENTIAL
 *  PlayStation(R)3 Programmer Tool Runtime Library 400.001
 *  Copyright (C) 2008 Sony Computer Entertainment Inc.
 *  All Rights Reserved.
 */

#ifndef __FSPATCH_H__
#define __FSPATCH_H__

// gcmutil
#include "gcmutil.h"
using namespace CellGcmUtil;

#include "FragmentPatch.h"

void fspatchPatchParamDirect(CGprogram prog, CGparameter param, void *addr, float *value, uint32_t len);
uint32_t fspatchGetParamIndex(CGprogram prog, CGparameter param);
uint32_t fspatchGetNamedParamIndex(CGprogram prog, const char* name);
uint32_t fspatchSetupFragmentPatch(CGprogram prog, Memory_t *patch_info, Memory_t *offset_list);
void fspatchSetPatchParam(PatchParam_t *param, const float* value);
void fspatchSetPatchParamDirect(void *ucode, PatchParam_t *param, const float* value);
void fspatchPatchParam(void *ucode, const PatchParam_t *params, uint32_t param_count);

uint32_t fspatchSetupFragmentPatchCgb(const CellCgbProgram *prog, Memory_t *patch_info, Memory_t *offset_list);

#endif /* __FSPATCH_H__ */
