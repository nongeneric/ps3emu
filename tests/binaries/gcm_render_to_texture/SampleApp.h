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
	virtual bool	onUpdate();
	virtual void	onRender();
	virtual	void	onShutdown();
	virtual void	onSize(const FWDisplayInfo & dispInfo);

	void initShader();
	void setTextureTarget();
	bool initStateBuffer(void);

private:
	enum {
		ARGB8 = 0,
		FP16,
		FP32,
	};

	static const uint32_t sLabelId = 128;

	FWTimeVal mLastTime;				// Last update time

	CGparameter mModelViewProj[2];		// Cg parameter for modelview matrix
	CGresource mPosIndex[2];			// Cg resource for vertex attribute
	CGresource mTexIndex[2];			// Cg resource for vertex attribute
	CGresource mTexUnit[2];				// Cg resource for texture sampler

	uint32_t mVertexCount;				// vertex count for a cube
	VertexData3D *mCubeVertices;		// vertices for a cube
	uint32_t mCubeOffset;				// offset of vertices for a cube
	VertexData2D *mQuadVertices;		// vertices for a quad
	uint32_t mQuadOffset;				// offset of vertices for a quad

	void *mTextureAddress;				// address of a static texture
	uint32_t mTextureOffset;			// offset of a static texture
	void *mRttColorAddress;				// address of color buffer
	uint32_t mRttColorOffset;			// offset of color buffer
	void *mRttDepthAddress;				// address of depth buffer
	uint32_t mRttDepthOffset;			// offset of depth buffer

	CGprogram mCGVertexProgram[2];		// Cg binary program
	CGprogram mCGFragmentProgram[2];	// Cg binary program

	void *mVertexProgramUCode[2];		// ucode of vertex programs
	void *mFragmentProgramUCode[2];		// ucode of fragment programs
	uint32_t mFragmentProgramOffset[2];	// offset of fragment programs

	uint32_t mTextureWidth;				// texture width
	uint32_t mTextureHeight;			// texture height
	uint32_t mTextureDepth;				// texture depth

	uint32_t mTextureType;				// texture type

	bool mEnableSwizzle;				// swizzled or linear
	uint32_t mPitch;					// pitch size for a rendered texture

	uint32_t mLabelValue;				// label value for synchronization

	uint32_t* mStateBufferAddress;
	uint32_t  mStateBufferOffset;
};

#endif // __SAMPLEAPP_H__
