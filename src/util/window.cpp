#include "window.h"

#include <cstdio>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <implot.h>

Window::Window(int width, int height, const std::string &title, bool vSync)
    : _width(width),
      _height(height),
      _title(title),
      _vSync(vSync)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
    {
        throw std::runtime_error("Could not initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(_width, _height, _title.c_str(), NULL, NULL);
    if (!_window)
    {
        glfwTerminate();
        throw std::runtime_error("Could not create GLFW window");
    }

    glfwMakeContextCurrent(_window);
    glfwSwapInterval(_vSync ? 1 : 0);

    glfwSetWindowUserPointer(_window, this);

    // input callbacks
    glfwSetFramebufferSizeCallback(_window, glfwFrameBufferSizeCallback);
    glfwSetKeyCallback(_window, glfwKeyboardCallback);
    glfwSetCursorPosCallback(_window, glfwCursorPosCallback);
    glfwSetMouseButtonCallback(_window, glfwMouseButtonCallback);
    glfwSetScrollCallback(_window, glfwScrollCallback);

    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Echec du chargement d'OpenGL (GLAD)");
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    _width = fbWidth;
    _height = fbHeight;

    // IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    // ImGui only shows debug stats: it must never touch the OS cursor. Besides being useless,
    // letting it set one breaks the cursor on Wayland -- GLFW keeps re-showing a window cursor
    // even in GLFW_CURSOR_DISABLED mode (its cursor animation timer ignores the cursor mode)
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();

    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool Window::shouldClose() { return glfwWindowShouldClose(_window); }

void Window::pollEvents() { glfwPollEvents(); }

void Window::swapBuffers() { glfwSwapBuffers(_window); }

float Window::getTime() const { return glfwGetTime(); }

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
    if (key < 0 || key >= GLFW_KEY_LAST)
        return;

    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        self->_keys[key] = true;
        self->_keysPressed[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        self->_keys[key] = false;
    }
}

void Window::glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

    if (self->_firstMouse)
    {
        self->_dx = 0;
        self->_dy = 0;
        self->_mouseX = xPos;
        self->_mouseY = yPos;
        self->_firstMouse = false;
        return;
    }

    self->_dx += xPos - self->_mouseX;
    self->_dy += self->_mouseY - yPos;
    self->_mouseX = xPos;
    self->_mouseY = yPos;
}

void Window::glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button < 0 || button >= GLFW_MOUSE_BUTTON_LAST)
        return;

    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        self->_buttons[button] = true;
        self->_buttonsPressed[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        self->_buttons[button] = false;
    }
}

void Window::glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    self->_scrollY += yOffset;
}

//* ======= INPUT HANDLING =========

namespace
{
int toGflwKey(Key key)
{
    switch (key)
    {
    case Key::W:
        return GLFW_KEY_W;
    case Key::A:
        return GLFW_KEY_A;
    case Key::S:
        return GLFW_KEY_S;
    case Key::D:
        return GLFW_KEY_D;
    case Key::Esc:
        return GLFW_KEY_ESCAPE;
    case Key::Space:
        return GLFW_KEY_SPACE;
    case Key::LShift:
        return GLFW_KEY_LEFT_SHIFT;
    case Key::LCtrl:
        return GLFW_KEY_LEFT_CONTROL;
    case Key::F3:
        return GLFW_KEY_F3;
    }

    return GLFW_KEY_UNKNOWN;
}

int toGlfwButton(MouseButton button)
{
    switch (button)
    {
    case MouseButton::Left:
        return GLFW_MOUSE_BUTTON_LEFT;
    case MouseButton::Right:
        return GLFW_MOUSE_BUTTON_RIGHT;
    case MouseButton::Middle:
        return GLFW_MOUSE_BUTTON_MIDDLE;
    }

    return -1;
}
} // namespace

bool Window::isKeyPressed(Key key) const { return _keys[toGflwKey(key)]; }

bool Window::consumeKeyPress(Key key)
{
    int keycode = toGflwKey(key);
    bool value = _keysPressed[keycode];
    _keysPressed[keycode] = false;
    return value;
}

bool Window::isButtonPressed(MouseButton button) const { return _buttons[toGlfwButton(button)]; }

bool Window::consumeButtonPress(MouseButton button)
{
    int code = toGlfwButton(button);
    bool value = _buttonsPressed[code];
    _buttonsPressed[code] = false;
    return value;
}

double Window::consumeDx()
{
    double tmp = _dx;
    _dx = 0.0;
    return tmp;
}

double Window::consumeDy()
{
    double tmp = _dy;
    _dy = 0.0;
    return tmp;
}

double Window::consumeScroll()
{
    double tmp = _scrollY;
    _scrollY = 0.0;
    return tmp;
}

glm::vec2 Window::getCursorPos() { return glm::vec2(_mouseX, _mouseY); }

void Window::resetMouse()
{
    _firstMouse = true;
    _dx = 0.0;
    _dy = 0.0;
}

void Window::setCursorEnabled(bool enabled)
{
    _cursorToggle = enabled;
    if (_cursorToggle)
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        resetMouse();
    }
}

bool Window::isCursorEnabled() { return _cursorToggle; }

void Window::enableInput() { _inputEnabled = true; }

void Window::disableInput()
{
    _inputEnabled = false;
    _keysPressed.fill(false);
    _buttonsPressed.fill(false);
}

bool Window::isInputEnabled() { return _inputEnabled; }
