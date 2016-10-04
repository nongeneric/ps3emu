/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef __SAMPLEAPP_H__
#define __SAMPLEAPP_H__

#include "FWGCMCamControlApplication.h"
#include "cellutil.h"

class SampleApp : public FWGCMCamControlApplication
{
public:
					SampleApp();
					~SampleApp();
	virtual bool	onInit(int argc, char **ppArgv);
	virtual void	onRender();
	virtual	void	onShutdown();
	virtual void	onSize(const FWDisplayInfo & dispInfo);

	void initShader(void);
	bool initStateBuffer(void);

private:
	static const uint32_t sLabelId = 128;

	FWTimeVal		mLastTime;

	CGparameter mModelViewProj;
	CGresource mPosIndex;
	CGresource mTexIndex;
	CGresource mTexUnit;

	VertexData3D *mVertexBuffer;
	uint32_t mVertexOffset;
	uint32_t mVertexCount;
	void *mTextureAddress;					// texture address
	uint32_t mTextureOffset;				// texture offset

	CGprogram mCGVertexProgram;				// CG binary program
	CGprogram mCGFragmentProgram;			// CG binary program

	void *mVertexProgramUCode;				// this is sysmem
	void *mFragmentProgramUCode;			// this is vidmem
	uint32_t mFragmentProgramOffset;

	uint32_t mTextureWidth;
	uint32_t mTextureHeight;
	uint32_t mTextureDepth;

	uint32_t *mTexBuffer;					// system mem texture

	float mCosTable[256*4];
	uint32_t mPalette[256];

	uint32_t *mLabel;
	uint32_t mLabelValue;

	uint32_t* mStateBufferAddress;
	uint32_t  mStateBufferOffset;
};

#endif // __SAMPLEAPP_H__
