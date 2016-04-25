/*   SCE CONFIDENTIAL                                       */
/*   PlayStation(R)3 Programmer Tool Runtime Library 400.001 */
/*   Copyright (C) 2006 Sony Computer Entertainment Inc.    */
/*   All Rights Reserved.                                   */

#ifndef __SAMPLEAPP_H__
#define __SAMPLEAPP_H__

#include "FWGCMCamControlApplication.h"
#include "cellutil.h"

const int NUM_OBJECTS = 3;

class SampleApp : public FWGCMCamControlApplication
{
public:
					SampleApp();
					~SampleApp();
	virtual bool	onInit(int argc, char **ppArgv);
	virtual void	onRender();
	virtual	void	onShutdown();
	virtual void	onSize(const FWDisplayInfo & dispInfo);
	virtual bool    onUpdate(void);

	typedef struct {
		void* address;
		uint32_t offset;
		uint32_t size;
	} AttributeBuffer;

	typedef struct {
		AttributeBuffer vert;
		AttributeBuffer normal;
		uint32_t v_count;
		Vector3  pos;
		float    color[4];
	} RenderObject;

private:
	void initShader();
	void DrawObject(RenderObject* object, Matrix4* M, Matrix4* IM, Matrix4* VP);
	void resetGeomData(void);
	void initGeomData(void);
	void initGeometry(void);
	bool initStateBuffer(void);

	static const float sDeadTime; // second
	static const uint32_t sLabelId = 128;

	FWTimeVal		mLastTime;

	FWInputFilter	*mpUp, *mpDown, *mpLeft, *mpRight;
	FWInputFilter	*mpL1, *mpL2, *mpR1, *mpR2, *mpTriangle;
	FWInputFilter	*mpStart, *mpSelect;

	CGparameter mModelMatrixParameter;
	CGparameter mViewProjMatrixParameter;

	CGparameter mCgClipPlane;

	CGresource mPosIndex;
	CGresource mNormIndex;
	CGresource mColorIndex;

	RenderObject mTorus[NUM_OBJECTS];
	RenderObject mSphere[NUM_OBJECTS];
	RenderObject mPlane;

	CGprogram mCGVertexProgram;				// CG binary program
	CGprogram mCGFragmentProgram;			// CG binary program

	void *mVertexProgramUCode;				// this is sysmem
	void *mFragmentProgramUCode;			// this is vidmem
	uint32_t mFragmentProgramOffset;

	uint64_t mDeadTimeCount;			// dead time of pad control
	Vector4 mClipPlane;		            // Plane equation: Ax+By+Cz+D=0

	uint32_t mClipPlaneFunc; 
	bool     mClipPlaneEnable;
	
	Point3 mCameraDefaultPos;			// default camera position
	float mCameraDefaultTilt;			// default camera tilt parameter
	float mCameraDefaultPan;			// default camera pan parameter

	// user selectable parameter
	int mSelectedParameter;				// 0=A, 1=B, 2=C, 3=D

	// boolean for object rotation on/off
	bool mObjectRotation;

	uint32_t* mStateBufferAddress;
	uint32_t  mStateBufferOffset;

	// lighting parameters
	CGparameter mLightPosParameter;
	CGparameter mEyePosParameter;

	// for fragment shading
	Vector3   mLightPos;           // light position in local space
};

#endif // __SAMPLEAPP_H__
