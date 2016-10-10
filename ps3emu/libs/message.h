#pragma once

#include "sys.h"
#include <boost/optional.hpp>

struct MessageCallbackInfo {
    uint32_t va;
    uint32_t args[2];
};

int32_t cellMsgDialogOpen2(uint32_t type,
                           cstring_ptr_t msgString,
                           uint32_t func,
                           uint32_t userData,
                           uint32_t extParam);

void emuMessageDraw(uint32_t width, uint32_t height);
boost::optional<MessageCallbackInfo> emuMessageFireCallback();
