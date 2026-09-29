#include "window.h"

#include <cstdio>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "graphics/gl/gl_debug.h"
#include "graphics/gl/gl_objects.h"

Window::Window(int width, int height, const std::string &title, bool vSync)
    : _width(width),
      _height(height),
      _title(title),
      _vSync(vSync),
      _window(createGlfwWindow(width, height, title)),
      _input(_window)
{
    glfwMakeContextCurrent(_window);
    glfwSwapInterval(_vSync ? 1 : 0);

    glfwSetWindowUserPointer(_window, this);

    // input callbacks
    glfwSetFramebufferSizeCallback(_window, glfwFrameBufferSizeCallback);
    glfwSetKeyCallback(_window, glfwKeyboardCallback);
    glfwSetCursorPosCallback(_window, glfwCursorPosCallback);
    glfwSetMouseButtonCallback(_window, glfwMouseButtonCallback);
    glfwSetScrollCallback(_window, glfwScrollCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Echec du chargement d'OpenGL (GLAD)");
    }

    // this thread owns the context: GL objects may only be created and destroyed here
    gl::setContextThread();

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // the callback runs inside the faulty GL call
    glDebugMessageCallback(gl::onDebugMessage, nullptr);
    // notifications are far too verbose
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

    // NDC depth goes from 0 to 1 instead of -1 to 1: required by the reverse-Z projection of
    // Camera, which would otherwise lose half of its precision. Every projection matrix must
    // therefore be built with the glm::*_ZO functions.
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    _width = fbWidth;
    _height = fbHeight;
}

GLFWwindow *Window::createGlfwWindow(int width, int height, const std::string &title)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
    {
        throw std::runtime_error("Could not initialize GLFW");
    }

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error("Could not create GLFW window");
    }

    return window;
}

Window::~Window()
{
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool Window::shouldClose() { return glfwWindowShouldClose(_window); }

void Window::pollEvents()
{
    // the events of the previous frame must be forgotten before GLFW delivers the new ones
    _input.beginFrame();
    glfwPollEvents();
}

void Window::swapBuffers() { glfwSwapBuffers(_window); }

double Window::getTime() const { return glfwGetTime(); }

//* ========== GLFW CALLBACKS ==========

void Window::glfwErrorCallback(int error_code, const char *description)
{
    fprintf(stderr, "GLFW Error: [%d] -- %s\n", error_code, description);
}

void Window::glfwFrameBufferSizeCallback(GLFWwindow *window, int width, int height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

    glViewport(0, 0, width, height);
    self->_width = width;
    self->_height = height;
}

void Window::glfwKeyboardCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->_input.onKey(key, action);
}

void Window::glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->_input.onCursorPos(xPos, yPos);
}

void Window::glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->_input.onMouseButton(button, action);
}

void Window::glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->_input.onScroll(yOffset);
}
