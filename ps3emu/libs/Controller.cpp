#include "Controller.h"
#include "ps3emu/log.h"
#include "ps3emu/utils.h"
#include <SDL2/SDL.h>

int32_t cellKbInit(uint32_t max_connect) {
    return CELL_KB_OK;
}

int32_t cellKbSetCodeType(uint32_t port_no, uint32_t type) {
    return CELL_KB_OK;
}

int32_t cellMouseInit(uint32_t max_connect) {
    return CELL_MOUSE_OK;
}

#define CELL_PAD_INFO_INTERCEPTED (1 << 0)

#define CELL_PAD_STATUS_ASSIGN_CHANGES (1 << 1)

#define CELL_PAD_SETTING_PRESS_ON (1 << 1)
#define CELL_PAD_SETTING_SENSOR_ON (1 << 2)
#define CELL_PAD_SETTING_PRESS_OFF (0)
#define CELL_PAD_SETTING_SENSOR_OFF (0)

#define CELL_PAD_CAPABILITY_PS3_CONFORMITY (1 << 0)
#define CELL_PAD_CAPABILITY_PRESS_MODE (1 << 1)
#define CELL_PAD_CAPABILITY_SENSOR_MODE (1 << 2)
#define CELL_PAD_CAPABILITY_HP_ANALOG_STICK (1 << 3)
#define CELL_PAD_CAPABILITY_ACTUATOR (1 << 4)

#define CELL_PAD_DEV_TYPE_STANDARD (0)
#define CELL_PAD_DEV_TYPE_BD_REMOCON (4)
#define CELL_PAD_DEV_TYPE_LDD (5)

std::string dualshock4Name = "Sony Computer Entertainment Wireless Controller";

auto mappings = {
    "030000004c050000c405000011810000,Sony Computer Entertainment Wireless Controller,platform:Linux,x:b3,a:b0,b:b1,y:b2,back:b8,guide:b10,start:b9,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,dpup:h0.1,leftshoulder:b4,lefttrigger:a2,rightshoulder:b5,righttrigger:a5,leftstick:b11,rightstick:b12,leftx:a0,lefty:a1,rightx:a3,righty:a4,platform:Linux"
};

int32_t cellPadInit(uint32_t max_connect) {
    for (auto mapping : mappings) {
        SDL_GameControllerAddMapping(mapping);
    }
    return CELL_PAD_OK;
}

bool isConnected(int i) {
    return SDL_IsGameController(i) &&
           SDL_GameControllerName(SDL_GameControllerOpen(i)) == dualshock4Name;
}

int32_t cellPadGetInfo2(CellPadInfo2* info) {
    info->max_connect = 1;
    info->now_connect = 1;
    info->system_info = 0;
    for (int i = 0; i < std::min(SDL_NumJoysticks(), CELL_PAD_MAX_PORT_NUM); ++i) {
        info->port_status[i] = isConnected(i) ? CELL_PAD_STATUS_CONNECTED
                                              : CELL_PAD_STATUS_DISCONNECTED;
        info->port_setting[i] = CELL_PAD_SETTING_PRESS_ON;
        info->device_capability[i] =
            CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE |
            CELL_PAD_CAPABILITY_HP_ANALOG_STICK;
        info->device_type[i] = CELL_PAD_DEV_TYPE_STANDARD;
    }
    return CELL_PAD_OK;
}

#define CELL_KB_STATUS_CONNECTED       (1)
#define CELL_KB_STATUS_DISCONNECTED    (0)

int32_t cellKbGetInfo(CellKbInfo* info) {
    info->max_connect = 1;
    info->now_connect = 0;
    for (int i = 0; i < CELL_KB_MAX_KEYBOARDS; ++i) {
        info->status[i] = CELL_KB_STATUS_DISCONNECTED;
    }
    return CELL_KB_OK;
}

int32_t cellMouseGetInfo(CellMouseInfo* info) {
    info->max_connect = 1;
    info->max_connect = 0;
    for (int i = 0; i < CELL_MAX_MICE; ++i) {
        info->status[i] = 0;
    }
    return CELL_MOUSE_OK;
}

int32_t cellPadEnd() {
    return CELL_OK;
}

#define CELL_PAD_ERROR_NO_DEVICE 0x80121107
#define SUINT2UCHAR(x) (uint16_t(x + 0x8000) >> 8)

int32_t cellPadGetData(uint32_t port_no, CellPadData* data) {
    SDL_GameControllerUpdate();
    auto handle = SDL_GameControllerOpen(port_no);
    
    memset(data->button, 0, sizeof(CellPadData));
    
    data->len = 8;
    data->button[0] = 0;
    data->button[1] = (7 << 4) | (data->len / 2);
    data->button[CELL_PAD_BTN_OFFSET_DIGITAL1] =
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? CELL_PAD_CTRL_LEFT : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? CELL_PAD_CTRL_DOWN : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? CELL_PAD_CTRL_RIGHT : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_DPAD_UP) ? CELL_PAD_CTRL_UP : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_START) ? CELL_PAD_CTRL_START : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_RIGHTSTICK) ? CELL_PAD_CTRL_R3 : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_LEFTSTICK) ? CELL_PAD_CTRL_L3 : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_GUIDE) ? CELL_PAD_CTRL_SELECT : 0);
    data->button[CELL_PAD_BTN_OFFSET_DIGITAL2] =
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_X) ? CELL_PAD_CTRL_SQUARE : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_A) ? CELL_PAD_CTRL_CROSS : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B) ? CELL_PAD_CTRL_CIRCLE : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y) ? CELL_PAD_CTRL_TRIANGLE : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? CELL_PAD_CTRL_R1 : 0) |
        (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? CELL_PAD_CTRL_L1 : 0) |
        (SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) == 0x7fff ? CELL_PAD_CTRL_R2 : 0) |
        (SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_TRIGGERLEFT) == 0x7fff ? CELL_PAD_CTRL_L2 : 0);
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] =
        SUINT2UCHAR(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_RIGHTX));
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] =
        SUINT2UCHAR(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_RIGHTY));
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] =
        SUINT2UCHAR(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTX));
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] =
        SUINT2UCHAR(SDL_GameControllerGetAxis(handle, SDL_CONTROLLER_AXIS_LEFTY));
        
    return CELL_OK;
}

int32_t cellKbEnd() {
    return CELL_OK;
}

int32_t cellMouseEnd() {
    return CELL_OK;
}

int32_t cellPadClearBuf(uint32_t port_no) {
    return CELL_OK;
}

int32_t cellPadSetPortSetting(uint32_t port_no, uint32_t port_setting) {
    return CELL_OK;
}

int32_t cellPadSetActDirect(uint32_t port_no, CellPadActParam* param) {
    return CELL_OK;
}
