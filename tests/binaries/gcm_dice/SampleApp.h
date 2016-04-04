/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 360.001 */
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

private:
	void initShader();
	void initSquares();
	void makeVertexTable();

private:
	FWTimeVal		mLastTime;

	CGparameter mModelViewProj;
	CGresource mPosIndex;
	CGresource mTexIndex;
	CGresource mColIndex;
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
	uint32_t *mTextures[6];
	uint32_t mTextureOffsets[6];

	uint32_t *mHeapBegin[2];
	uint32_t mHeapIdx;

	float *mVertexTable;
	uint32_t mVertexTableOffset;

	float mCosTable[256];

public:
	square mSquareTable[SQUARE_COUNT];

};

#endif // __SAMPLEAPP_H__
