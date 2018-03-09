#include "graphics.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdexcept>

void Window::init() {
#ifdef GL_DEBUG_ENABLED
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GL_TRUE);
#endif
    _window = glfwCreateWindow(width(), height(), "ps3emu", NULL, NULL);
    if (!_window) {
        throw std::runtime_error("window creation failed");
    }
    glfwMakeContextCurrent(_window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("opengl function loading failed");
    }

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    if (major < 4 || (major == 4 && minor < 5)) {
        throw std::runtime_error("bad opengl version");
    }

    glfwSwapInterval(-1);
}

void Window::shutdown() {
    glfwTerminate();
}

void Window::swapBuffers() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

unsigned Window::width() {
    return 1280;
}

unsigned Window::height() {
    return 720;
}
