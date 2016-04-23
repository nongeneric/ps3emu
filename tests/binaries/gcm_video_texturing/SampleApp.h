/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef __SAMPLEAPP_H__
#define __SAMPLEAPP_H__

#include "FWGCMCamControlApplication.h"
#include "cellutil.h"

#define NUM_CIRCLE 100

typedef struct {
	int32_t x_center;
	int32_t y_center;
	int32_t x_velocity;
	int32_t y_velocity;
	int32_t radius;
	int32_t radius_max;
	int32_t scale;
	uint32_t color;
} CIRCLE_T;

class SampleApp : public FWGCMCamControlApplication
{
public:
					SampleApp();
					~SampleApp();
	virtual bool	onInit(int argc, char **ppArgv);
	virtual bool	onUpdate();
	virtual void	onRender();

private:
	void initShader();
	bool initStateBuffer(void);
	void initCircle(void);
	void updateCircle(void);
	void updateTexture(void);

	static const uint32_t sLabelId = 128;

private:
	FWTimeVal		mLastTime;

	CGresource mSampler;
	CGresource mObjCoordIndex;
	CGresource mTexCoordIndex;

	CGprogram mCGVertexProgram;				// CG binary program
	CGprogram mCGFragmentProgram;			// CG binary program

	CGparameter mObjCoord;
	CGparameter mTexCoord;

	void *mVertexProgramUCode;				// this is sysmem
	void *mFragmentProgramUCode;			// this is vidmem
	uint32_t mFragmentProgramOffset;

	VertexData3D *mVertexBuffer;            // vertex buffer address
	uint32_t mVertexBufferOffset;                 // vertex buffer offset

	void *mTextureAddress;					// texture address
	uint32_t mTextureOffset;				// texture offset

	CellGcmTexture mTexture;                     // texture structure

	uint32_t *mLabel;
	uint32_t mLabelValue;

	uint32_t* mStateBufferAddress;
	uint32_t  mStateBufferOffset;

	CIRCLE_T mCircle[NUM_CIRCLE];
};

#endif // __SAMPLEAPP_H__
