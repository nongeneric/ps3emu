/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#ifndef _MAIN_H_
#define _MAIN_H_

#include "FWGLCamControlApplication.h"
#include "grid.h"
#include "main.h"

float *poissonSolver(void);

class PoissonViewer :
	public FWGLCamControlApplication
{
public:
	PoissonViewer();
	virtual bool onInit(int argc, char **ppArgv);
	virtual void onRender();
	virtual bool onUpdate();
	virtual void onShutdown();
	virtual void onSize(const FWDisplayInfo &dispInfo);

private:
	FWInputFilter *mpSpaceKey, *mpMouseX, *mpMouseY, *mpMouseLeft, *mpMouseMiddle, *mpMouseRight;
	FWInputFilter *mpLeftX, *mpLeftY, *mpRightX, *mpRightY;
	FWInputFilter *mpUp, *mpRight, *mpDown, *mpLeft;
	FWInputFilter *mpTriangle, *mpCircle, *mpCross, *mpSquare;
	FWInputFilter *mpL1, *mpL2, *mpL3;
	FWInputFilter *mpR1, *mpR2, *mpR3;
	FWInputFilter *mpSelect, *mpStart;

	bool mLastStateUp;
	bool mLastStateDown;

	FWTimeVal mLastTime;
	Grid mGrid;
};

#endif //_MAIN_H_

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
