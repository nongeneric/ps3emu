/* SCE CONFIDENTIAL
 * PlayStation(R)Edge 1.3.0
 * Copyright (C) 2007 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */


#ifndef __SAMPLEAPP_H__
#define __SAMPLEAPP_H__

#include "spu/job_geom_interface.h"

#include "gcm/FWGCMCamControlApplication.h"
#include "FWInput.h"
#include "FWTime.h"
#include "cellutil.h"

class SampleApp : public FWGCMCamApplication
{
public:
					SampleApp();
					~SampleApp();
	virtual bool	onInit(int argc, char **ppArgv);
	virtual void	onRender();
	virtual bool	onUpdate();
	virtual	void	onShutdown();
	virtual void	onSize(const FWDisplayInfo & dispInfo);

private:
	void	initShader();
	void	CreateGeomJob(JobGeom256* job, CellGcmContextData *ctx, EdgeGeomPpuConfigInfo *info, void *skinningMatrices, uint32_t elephantIndex) const;

private:
	FWTimeVal					mLastTime;
	FWInputFilter				*mpSquare, *mpCross, *mpStart, *mpTriangle, *mpCircle;
	FWInputFilter				*mpUp, *mpDown;
	FWInputFilter				*mpInputX0, *mpInputY0, *mpInputX1, *mpInputY1;
	FWTimeVal					mLastUpdate;
	float						mTurnRate;
	float						mMoveRate;

	float						*mSkinningMatrices;
	void						*mTextures[2];
	void						*mCommandBuffer;
	uint32_t					mCommandBufferOffset;

	uint8_t						*mSharedOutputBuffer;
	uint8_t						*mRingOutputBuffer;
	EdgeGeomOutputBufferInfo	mOutputBufferInfo;		// FIXME: warning - must be 128-byte aligned for atomic access
	EdgeGeomViewportInfo		mViewportInfo;
	// EdgeGeomLocalToWorldMatrix	mLocalToWorldMatrix; // fragment-patch-sample has its own local-to-world matrix per elephant

	CGparameter					mModelViewProj;
	CGprogram					mCGVertexProgram;				// CG binary program
	CGprogram					mCGFragmentProgram;				// CG binary program
	void						*mVertexProgramUCode;			// this is sysmem
	void						*mFragmentProgramUCode;			// this is vidmem
	uint32_t					mFragmentProgramOffset;

	uint32_t					mEdgeJobSize;
	void						*mEdgeJobStart;
	uint32_t					mSendEventJobSize;
	void						*mSendEventJobStart;

	CellSpurs					mSpurs;
	CellSpursEventFlag			mSpursEventFlag;
};

#endif // __SAMPLEAPP_H__
