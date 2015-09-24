#pragma once

#include <stdint.h>

class GLFWwindow;
class Window {
    GLFWwindow* _window;
public:
    void Init();
};