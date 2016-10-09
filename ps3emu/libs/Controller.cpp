#include "Controller.h"
#include <GLFW/glfw3.h>

int32_t cellPadInit(uint32_t max_connect) {
    return CELL_PAD_OK;
}

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

#define CELL_PAD_STATUS_DISCONNECTED (0)
#define CELL_PAD_STATUS_CONNECTED (1 << 0)
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

int32_t cellPadGetInfo2(CellPadInfo2* info) {
    info->max_connect = 1;
    info->now_connect = 1;
    info->system_info = 0;
    for (int i = 0; i < CELL_PAD_MAX_PORT_NUM; ++i) {
        static_assert(GLFW_JOYSTICK_1 == 0, "");
        info->port_status[i] = glfwJoystickPresent(i) ? CELL_PAD_STATUS_CONNECTED
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

#define CELL_PAD_CTRL_LEFT      (1 << 7)
#define CELL_PAD_CTRL_DOWN      (1 << 6)
#define CELL_PAD_CTRL_RIGHT     (1 << 5)
#define CELL_PAD_CTRL_UP        (1 << 4)
#define CELL_PAD_CTRL_START     (1 << 3)
#define CELL_PAD_CTRL_R3        (1 << 2)
#define CELL_PAD_CTRL_L3        (1 << 1)
#define CELL_PAD_CTRL_SELECT    (1 << 0)

#define CELL_PAD_CTRL_SQUARE    (1 << 7)
#define CELL_PAD_CTRL_CROSS     (1 << 6)
#define CELL_PAD_CTRL_CIRCLE    (1 << 5)
#define CELL_PAD_CTRL_TRIANGLE  (1 << 4)
#define CELL_PAD_CTRL_R1        (1 << 3)
#define CELL_PAD_CTRL_L1        (1 << 2)
#define CELL_PAD_CTRL_R2        (1 << 1)
#define CELL_PAD_CTRL_L2        (1 << 0)

enum GlfwDS4Buttons {
    GLFW_DS4_BUTTON_SQUARE = 0,
    GLFW_DS4_BUTTON_CROSS,
    GLFW_DS4_BUTTON_CIRCLE,
    GLFW_DS4_BUTTON_TRIANGLE,
    GLFW_DS4_BUTTON_L1,
    GLFW_DS4_BUTTON_R1,
    GLFW_DS4_BUTTON_L2,
    GLFW_DS4_BUTTON_R2,
    GLFW_DS4_BUTTON_SELECT,
    GLFW_DS4_BUTTON_START,
    GLFW_DS4_BUTTON_L3,
    GLFW_DS4_BUTTON_R3,
    GLFW_DS4_BUTTON_PS,
    GLFW_DS4_BUTTON_TOUCHPAD
};

enum GlfwDS4Axes {
    GLFW_DS4_AXIS_LEFT_X = 0,
    GLFW_DS4_AXIS_LEFT_Y,
    GLFW_DS4_AXIS_RIGHT_X,
    GLFW_DS4_AXIS_L2,
    GLFW_DS4_AXIS_R2,
    GLFW_DS4_AXIS_RIGHT_Y,
    GLFW_DS4_AXIS_LEFT_RIGHT,
    GLFW_DS4_AXIS_DOWN_UP
};

enum CellPadButtonDataOffset {
    CELL_PAD_BTN_OFFSET_DIGITAL1 = 2,
    CELL_PAD_BTN_OFFSET_DIGITAL2 = 3,
    CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X = 4,
    CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y = 5,
    CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X  = 6,
    CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y  = 7,
    CELL_PAD_BTN_OFFSET_PRESS_RIGHT    = 8,
    CELL_PAD_BTN_OFFSET_PRESS_LEFT     = 9,
    CELL_PAD_BTN_OFFSET_PRESS_UP       = 10,
    CELL_PAD_BTN_OFFSET_PRESS_DOWN     = 11,
    CELL_PAD_BTN_OFFSET_PRESS_TRIANGLE = 12,
    CELL_PAD_BTN_OFFSET_PRESS_CIRCLE   = 13,
    CELL_PAD_BTN_OFFSET_PRESS_CROSS    = 14,
    CELL_PAD_BTN_OFFSET_PRESS_SQUARE   = 15,
    CELL_PAD_BTN_OFFSET_PRESS_L1       = 16,
    CELL_PAD_BTN_OFFSET_PRESS_R1       = 17,
    CELL_PAD_BTN_OFFSET_PRESS_L2       = 18,
    CELL_PAD_BTN_OFFSET_PRESS_R2       = 19,
    CELL_PAD_BTN_OFFSET_SENSOR_X       = 20,
    CELL_PAD_BTN_OFFSET_SENSOR_Y       = 21,
    CELL_PAD_BTN_OFFSET_SENSOR_Z       = 22,
    CELL_PAD_BTN_OFFSET_SENSOR_G       = 23
};

#define FLOAT2INT(x) (uint16_t)((x + 1.f) / 2.f * 255.f)

int32_t cellPadGetData(uint32_t port_no, CellPadData* data) {
    int buttonCount, axesCount;
    auto buttons = glfwGetJoystickButtons(port_no, &buttonCount);
    auto axes = glfwGetJoystickAxes(port_no, &axesCount);
    
    data->len = 8;
    data->button[0] = 0;
    data->button[1] = (7 << 4) | (data->len / 2);
    data->button[CELL_PAD_BTN_OFFSET_DIGITAL1] =
        (axes[GLFW_DS4_AXIS_LEFT_RIGHT] == -1.f ? CELL_PAD_CTRL_LEFT : 0) |
        (axes[GLFW_DS4_AXIS_DOWN_UP] == 1.f ? CELL_PAD_CTRL_DOWN : 0) |
        (axes[GLFW_DS4_AXIS_LEFT_RIGHT] == 1.f ? CELL_PAD_CTRL_RIGHT : 0) |
        (axes[GLFW_DS4_AXIS_DOWN_UP] == -1.f ? CELL_PAD_CTRL_UP : 0) |
        (buttons[GLFW_DS4_BUTTON_START] ? CELL_PAD_CTRL_START : 0) |
        (buttons[GLFW_DS4_BUTTON_R3] ? CELL_PAD_CTRL_R3 : 0) |
        (buttons[GLFW_DS4_BUTTON_L3] ? CELL_PAD_CTRL_L3 : 0) |
        (buttons[GLFW_DS4_BUTTON_SELECT] ? CELL_PAD_CTRL_SELECT : 0);
    data->button[CELL_PAD_BTN_OFFSET_DIGITAL2] =
        (buttons[GLFW_DS4_BUTTON_SQUARE] ? CELL_PAD_CTRL_SQUARE : 0) |
        (buttons[GLFW_DS4_BUTTON_CROSS] ? CELL_PAD_CTRL_CROSS : 0) |
        (buttons[GLFW_DS4_BUTTON_CIRCLE] ? CELL_PAD_CTRL_CIRCLE : 0) |
        (buttons[GLFW_DS4_BUTTON_TRIANGLE] ? CELL_PAD_CTRL_TRIANGLE : 0) |
        (buttons[GLFW_DS4_BUTTON_R1] ? CELL_PAD_CTRL_R1 : 0) |
        (buttons[GLFW_DS4_BUTTON_L1] ? CELL_PAD_CTRL_L1 : 0) |
        (buttons[GLFW_DS4_BUTTON_R2] ? CELL_PAD_CTRL_R2 : 0) |
        (buttons[GLFW_DS4_BUTTON_L2] ? CELL_PAD_CTRL_L2 : 0);
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] =
        FLOAT2INT(axes[GLFW_DS4_AXIS_RIGHT_X]);
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] =
        FLOAT2INT(axes[GLFW_DS4_AXIS_RIGHT_Y]);
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] =
        FLOAT2INT(axes[GLFW_DS4_AXIS_LEFT_X]);
    data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] =
        FLOAT2INT(axes[GLFW_DS4_AXIS_LEFT_Y]);
        
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
