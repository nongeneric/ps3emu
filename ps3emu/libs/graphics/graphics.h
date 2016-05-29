#pragma once

#include <stdint.h>

class GLFWwindow;
class Window {
    GLFWwindow* _window;
public:
    void init();
    void shutdown();
    void swapBuffers();
    unsigned width();
    unsigned height();
};