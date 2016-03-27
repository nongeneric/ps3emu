#include "graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

void Window::init() {
    if (!glfwInit()) {
        throw std::runtime_error("glfw initialization failed");
    }
    _window = glfwCreateWindow(width(), height(), "ps3emu", NULL, NULL);
    if (!_window) {
        throw std::runtime_error("window creation failed");
    }
    glfwMakeContextCurrent(_window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void Window::shutdown() {
    glfwTerminate();
}

void Window::swapBuffers() {
    glfwSwapBuffers(_window);
}

unsigned Window::width() {
    return 1280;
}

unsigned Window::height() {
    return 720;
}