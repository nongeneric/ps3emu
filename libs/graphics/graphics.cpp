#include "graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

void Window::Init() {
    if (!glfwInit()) {
        throw std::runtime_error("glfw initialization failed");
    }
    _window = glfwCreateWindow(1280, 720, "ps3emu", NULL, NULL);
    if (!_window) {
        throw std::runtime_error("window creation failed");
    }
    glfwMakeContextCurrent(_window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void Window::SwapBuffers() {
    glfwSwapBuffers(_window);
}
