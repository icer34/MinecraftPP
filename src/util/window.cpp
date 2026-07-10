#include "window.h"

#include <cstdio>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

Window::Window(int width, int height, const std::string &title, bool vSync) 
    : m_width(width), m_height(height), m_title(title), m_vSync(vSync)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if(!glfwInit()) {
        throw std::runtime_error("Could not initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);
    if(!m_window) {
        glfwTerminate();
        throw std::runtime_error("Could not create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(m_vSync ? 1 : 0);

    glfwSetWindowUserPointer(m_window, this);

    //input callbacks
    glfwSetFramebufferSizeCallback(m_window, glfwFrameBufferSizeCallback);
    glfwSetKeyCallback(m_window, glfwKeyboardCallback);
    glfwSetCursorPosCallback(m_window, glfwCursorPosCallback);
    glfwSetMouseButtonCallback(m_window, glfwMouseButtonCallback);

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Echec du chargement d'OpenGL (GLAD)");
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    //IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

Window::~Window() 
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() 
{
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() 
{
    glfwPollEvents();
}

void Window::swapBuffers() 
{
    glfwSwapBuffers(m_window);
}

float Window::getTime() const
{
    return glfwGetTime();
}

float Window::getAspectRatio() const
{
    return (float) m_width / m_height;
}

//* ========== GLFW CALLBACKS ==========

void Window::glfwErrorCallback(int error_code, const char* description) 
{
    fprintf(stderr, "GLFW Error: [%d] -- %s\n", error_code, description);
}

void Window::glfwFrameBufferSizeCallback(GLFWwindow* window, int width, int height) 
{
    if(width == 0 || height == 0) {
        return;
    }

    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    glViewport(0, 0, width, height);
    self->m_width = width;
    self->m_height = height;
}

void Window::glfwKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(key < 0 || key >= GLFW_KEY_LAST) return;

    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
            self->m_keys[key] = true;
            self->m_keysPressed[key] = true;
        } else if (action == GLFW_RELEASE) {
            self->m_keys[key] = false;
        }
}

void Window::glfwCursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if(self->m_firstMouse) {
            self->m_dx = 0;
            self->m_dy = 0;
            self->m_mouseX = xPos;
            self->m_mouseY = yPos;
            self->m_firstMouse = false;
            return;
        }

        self->m_dx += xPos - self->m_mouseX;
        self->m_dy += self->m_mouseY - yPos;
        self->m_mouseX = xPos;
        self->m_mouseY = yPos;
}

void Window::glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button < 0 || button >= GLFW_MOUSE_BUTTON_LAST) return;

    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

    if(action == GLFW_PRESS) {
            self->m_buttons[button] = true;
            self->m_buttonsPressed[button] = true;
        } else if(action == GLFW_RELEASE) {
            self->m_buttons[button] = false;
        }
}

//* ======= INPUT HANDLING =========

namespace {
    int toGflwKey(Key key)
    {
        switch(key)
        {
            case Key::W : return GLFW_KEY_W;
            case Key::A : return GLFW_KEY_A;
            case Key::S : return GLFW_KEY_S;
            case Key::D : return GLFW_KEY_D;
            case Key::Esc : return GLFW_KEY_ESCAPE;
            case Key::Space : return GLFW_KEY_SPACE;
            case Key::LShift : return GLFW_KEY_LEFT_SHIFT;
            case Key::LCtrl : return GLFW_KEY_LEFT_CONTROL;
        }

        return GLFW_KEY_UNKNOWN;
    }

    int toGlfwButton(MouseButton button)
    {
        switch(button)
        {
            case MouseButton::Left : return GLFW_MOUSE_BUTTON_LEFT;
            case MouseButton::Right : return GLFW_MOUSE_BUTTON_RIGHT;
            case MouseButton::Middle : return GLFW_MOUSE_BUTTON_MIDDLE;
        }

        return -1;
    }
}

bool Window::isKeyPressed(Key key) const
{
    return m_keys[toGflwKey(key)];
}

bool Window::consumeKeyPress(Key key)
{
    int keycode = toGflwKey(key);
    bool value = m_keysPressed[keycode];
    m_keysPressed[keycode] = false;
    return value;
}

bool Window::isButtonPressed(MouseButton button) const
{
    return m_buttons[toGlfwButton(button)];
}

bool Window::consumeButtonPress(MouseButton button)
{
    int code = toGlfwButton(button);
    bool value = m_buttonsPressed[code];
    m_buttonsPressed[code] = false;
    return value;
}

double Window::consumeDx()
{
    double tmp = m_dx;
    m_dx = 0.0;
    return tmp;
}

double Window::consumeDy()
{
    double tmp = m_dy;
    m_dy = 0.0;
    return tmp;
}

void Window::resetMouse()
{
    m_firstMouse = true;
    m_dx = 0.0;
    m_dy = 0.0;
}

void Window::toggleCursor()
{
    m_cursorToggle = !m_cursorToggle;
    if (m_cursorToggle) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        resetMouse();
    }
}

bool Window::isCursorEnabled()
{
    return m_cursorToggle;
}

void Window::enableInput()
{
    m_inputEnabled = true;
}

void Window::disableInput()
{
    m_inputEnabled = false;
    m_keysPressed.fill(false);
    m_buttonsPressed.fill(false);
}

bool Window::isInputEnabled()
{
    return m_inputEnabled;
}

//* ========== IMGUI ==========

void Window::beginImguiFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Window::endImguiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
