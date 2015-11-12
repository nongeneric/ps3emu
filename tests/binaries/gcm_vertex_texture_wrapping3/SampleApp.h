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
	void createFloatHeightTexture(void);
	void updateTexture( void );
	bool initStateBuffer(void);

private:
	static const uint32_t sRow = 100;
	static const uint32_t sColumn = 100;
	static const float sDeadTime; // second

	FWTimeVal		mLastTime;
	FWInputFilter	*mpSquare, *mpCross, *mpStart, *mpTriangle, *mpCircle;
	FWInputFilter	*mpUp, *mpDown;

	uint32_t mVertexCount;				// vertex count
	CGparameter mModelViewProj;			// model-view-projection matrix
	CGparameter mVertTexture;			// model-view-projection matrix
	CGparameter mCgAnimation;			// model-view-projection matrix
	CGresource mPosIndex;				// attribute index for position
	CGresource mTexIndex;				// attribute index for texture coord
	CGresource mTexUnit;				// attribute index for texture unit

	VertexData3D *mVertexBuffer;		// vertex buffer for plane
	uint32_t mVertexOffset;				// offset of vertex buffer for plane
	uint32_t* mIndicesBuffer;           // index buffer for plane
	uint32_t mIndicesOffset;            // offset of index buffer for plane 
	void *mTextureAddress;				// texture address
	uint32_t mTextureOffset;			// texture offset

	CGprogram mCGVertexProgram;			// CG binary program
	CGprogram mCGFragmentProgram;		// CG binary program

	void *mVertexProgramUCode;			// this is sysmem
	void *mFragmentProgramUCode;		// this is vidmem
	uint32_t mFragmentProgramOffset;

	Point3 mCameraDefaultPos;			// default camera position
	float mCameraDefaultTilt;			// default camera tilt parameter
	float mCameraDefaultPan;			// default camera pan parameter

	uint64_t mDeadTimeCount;			// dead time of pad control

	uint8_t  mDrawPrimitive;            // triangle strip / line strip

	Vector4  mAnimationParameter;       // do animation in vertex shader
	float mHeightFactor;


	// Textures
	CellGcmTexture mTexture;

	// Texture type selector (W32_Z32_Y32_X32 or X32)
	bool mIsTexSingleComponent;
	bool mAnimate;

	uint32_t* mStateBufferAddress;
	uint32_t  mStateBufferOffset;
};


#endif // __SAMPLEAPP_H__
