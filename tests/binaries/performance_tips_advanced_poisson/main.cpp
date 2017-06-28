/* SCE CONFIDENTIAL                                    */
/* PlayStation(R)3 Programmer Tool Runtime Library 400.001                                           */
/* Copyright (C) 2007 Sony Computer Entertainment Inc. */
/* All Rights Reserved.                                */

#include <stdio.h>
#include "main.h"
#include "FWDebugFont.h"
#include "FWTime.h"
#include "common.h"

// instantiate the class
PoissonViewer app;

PoissonViewer::PoissonViewer()
{
	// set window title for Win32/Linux
	strcpy(mStartupInfo.mWindowTitle, "Poisson Viewer");

	// enable antialiasing
	mDispInfo.mAntiAlias = true;

	// initialize Pad state
	mLastStateUp = false;
	mLastStateDown = false;
}

bool PoissonViewer::onInit(int argc, char **ppArgv)
{
	FWGLCamControlApplication::onInit(argc, ppArgv);

	// get pointer to keyboard device
	FWInputDevice *pKeybd = FWInput::getDevice(FWInput::DeviceType_Keyboard, 0);

	// bind a filter to the space key
	if (pKeybd != NULL) {
		mpSpaceKey = pKeybd->bindFilter();
		mpSpaceKey->setChannel(FWInput::Channel_Key_Space);
	}
	else {
		mpSpaceKey = NULL;
	}

	FWInputDevice *pMouse = FWInput::getDevice(FWInput::DeviceType_Mouse, 0);

	if (pMouse != NULL) {
		((FWInputDeviceMouse *)pMouse)->setClientRelative(true);
		mpMouseX = pMouse->bindFilter();
		mpMouseX->setChannel(FWInput::Channel_XAxis_0);

		mpMouseY = pMouse->bindFilter();
		mpMouseY->setChannel(FWInput::Channel_YAxis_0);

		mpMouseLeft = pMouse->bindFilter();
		mpMouseLeft->setChannel(FWInput::Channel_Button_LeftMB);

		mpMouseMiddle = pMouse->bindFilter();
		mpMouseMiddle->setChannel(FWInput::Channel_Button_MiddleMB);

		mpMouseRight = pMouse->bindFilter();
		mpMouseRight->setChannel(FWInput::Channel_Button_RightMB);
	}
	else {
		mpMouseX = mpMouseY = mpMouseLeft = mpMouseMiddle = mpMouseRight = NULL;
	}

	FWInputDevice *pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if (pPad != NULL) {
		mpLeftX = pPad->bindFilter();
		mpLeftX->setChannel(FWInput::Channel_XAxis_0);
		mpLeftY = pPad->bindFilter();
		mpLeftY->setChannel(FWInput::Channel_YAxis_0);
		mpRightX = pPad->bindFilter();
		mpRightX->setChannel(FWInput::Channel_XAxis_1);
		mpRightY = pPad->bindFilter();
		mpRightY->setChannel(FWInput::Channel_YAxis_1);
		mpUp = pPad->bindFilter();
		mpUp->setChannel(FWInput::Channel_Button_Up);
		mpRight = pPad->bindFilter();
		mpRight->setChannel(FWInput::Channel_Button_Right);
		mpDown = pPad->bindFilter();
		mpDown->setChannel(FWInput::Channel_Button_Down);
		mpLeft = pPad->bindFilter();
		mpLeft->setChannel(FWInput::Channel_Button_Left);
		mpTriangle = pPad->bindFilter();
		mpTriangle->setChannel(FWInput::Channel_Button_Triangle);
		mpCircle = pPad->bindFilter();
		mpCircle->setChannel(FWInput::Channel_Button_Circle);
		mpCross = pPad->bindFilter();
		mpCross->setChannel(FWInput::Channel_Button_Cross);
		mpSquare = pPad->bindFilter();
		mpSquare->setChannel(FWInput::Channel_Button_Square);
		mpL1 = pPad->bindFilter();
		mpL1->setChannel(FWInput::Channel_Button_L1);
		mpL2 = pPad->bindFilter();
		mpL2->setChannel(FWInput::Channel_Button_L2);
		mpL3 = pPad->bindFilter();
		mpL3->setChannel(FWInput::Channel_Button_L3);
		mpR1 = pPad->bindFilter();
		mpR1->setChannel(FWInput::Channel_Button_R1);
		mpR2 = pPad->bindFilter();
		mpR2->setChannel(FWInput::Channel_Button_R2);
		mpR3 = pPad->bindFilter();
		mpR3->setChannel(FWInput::Channel_Button_R3);
		mpSelect = pPad->bindFilter();
		mpSelect->setChannel(FWInput::Channel_Button_Select);
		mpStart = pPad->bindFilter();
		mpStart->setChannel(FWInput::Channel_Button_Start);
	}

	// print command line args to the debug console
	for (int i = 0; i < argc; i++) {
		FWDebugConsole::print(ppArgv[i]);
	}

	// set default camera position
	mCamera.setPosition(Point3(0.f, 30.f, 150.f));

	// setup grid
	mGrid.setNumGridLines(NX, NY);
	mGrid.setHeightMap(poissonSolver());
	mGrid.setSpacing(100.0f / NX);
	return true;
}

bool PoissonViewer::onUpdate()
{
	// base implementation
	FWGLCamControlApplication::onUpdate();

	// change scaling magnitude
	bool newStateUp = mpUp->getBoolValue();
	if (newStateUp && !mLastStateUp) {
		mGrid.setHeightScale(2.0f * mGrid.getHeightScale());
	}

	mLastStateUp = newStateUp;

	bool newStateDown = mpDown->getBoolValue();
	if (newStateDown && !mLastStateDown) {
		mGrid.setHeightScale(0.5f * mGrid.getHeightScale());
	}

	mLastStateDown = newStateDown;

	return true;
}

int frame = 0;

void PoissonViewer::onRender()
{
	if (frame == 2) {
		cellGcmFinish(gCellGcmCurrentContext, 0x1313);
		exit(0);
	}
	// base implementation clears screen and sets up camera
	FWGLCamControlApplication::onRender();

	// render grid
	mGrid.render();
	frame++;
}

void PoissonViewer::onSize(const FWDisplayInfo &rDispInfo)
{
	FWGLCamControlApplication::onSize(rDispInfo);
}

void PoissonViewer::onShutdown()
{
	FWGLCamControlApplication::onShutdown();

	// unbind input filters
	FWInputDevice *pKeybd = FWInput::getDevice(FWInput::DeviceType_Keyboard, 0);

	if (pKeybd != NULL) {
		pKeybd->unbindFilter(mpSpaceKey);
	}

	FWInputDevice *pMouse = FWInput::getDevice(FWInput::DeviceType_Mouse, 0);

	if (pMouse != NULL) {
		pMouse->unbindFilter(mpMouseX);
		pMouse->unbindFilter(mpMouseY);
		pMouse->unbindFilter(mpMouseLeft);
		pMouse->unbindFilter(mpMouseMiddle);
		pMouse->unbindFilter(mpMouseRight);
	}

	FWInputDevice *pPad = FWInput::getDevice(FWInput::DeviceType_Pad, 0);

	if (pPad != NULL) {
		pPad->unbindFilter(mpLeftX);
		pPad->unbindFilter(mpLeftY);
		pPad->unbindFilter(mpRightX);
		pPad->unbindFilter(mpRightY);
		pPad->unbindFilter(mpUp);
		pPad->unbindFilter(mpRight);
		pPad->unbindFilter(mpDown);
		pPad->unbindFilter(mpLeft);
		pPad->unbindFilter(mpTriangle);
		pPad->unbindFilter(mpCircle);
		pPad->unbindFilter(mpCross);
		pPad->unbindFilter(mpSquare);
		pPad->unbindFilter(mpL1);
		pPad->unbindFilter(mpL2);
		pPad->unbindFilter(mpL3);
		pPad->unbindFilter(mpR1);
		pPad->unbindFilter(mpR2);
		pPad->unbindFilter(mpR3);
		pPad->unbindFilter(mpSelect);
		pPad->unbindFilter(mpStart);
	}
}

/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * tab-width: 4
 * End:
 * vim:sw=4:sts=4:ts=4
 */
