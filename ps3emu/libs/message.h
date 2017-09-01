#pragma once

#include "sys.h"
#include <optional>

int32_t cellMsgDialogOpen2(uint32_t type,
                           cstring_ptr_t msgString,
                           uint32_t func,
                           uint32_t userData,
                           uint32_t extParam);

int32_t cellMsgDialogProgressBarSetMsg(uint32_t progressbarIndex,
                                       cstring_ptr_t msgString);

int32_t cellMsgDialogClose(float delayTime);

int32_t cellMsgDialogProgressBarInc(uint32_t progressbarIndex, uint32_t delta);

int32_t cellMsgDialogAbort();


void emuMessageDraw(uint32_t width, uint32_t height);
