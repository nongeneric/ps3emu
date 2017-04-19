#include "graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

void Window::init() {
    _window = glfwCreateWindow(width(), height(), "ps3emu", NULL, NULL);
    if (!_window) {
        throw std::runtime_error("window creation failed");
    }
    glfwMakeContextCurrent(_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("opengl function loading failed");
    }
    glfwSwapInterval(-1);
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
