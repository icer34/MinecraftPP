#pragma once

#include <string>
#include <array>

#include "key_codes.h"

struct GLFWwindow;

class Window 
{
public:
    Window(int width, int height, const std::string &title, bool vSync);
    ~Window();

    void pollEvents();
    void swapBuffers();
    bool shouldClose();

    float getTime() const;
    float getAspectRatio() const;

    //* INPUT HANDLING
    bool isKeyPressed(Key key) const;
    bool consumeKeyPress(Key key);

    bool isButtonPressed(MouseButton button) const;
    bool consumeButtonPress(MouseButton button);

    double consumeDx();
    double consumeDy();

    void resetMouse();

    void toggleCursor();

    void enableInput();
    void disableInput();
    bool isInputEnabled();

    //* IMGUI METHODS
    void beginImguiFrame();
    void endImguiFrame();

private:
    int m_width;
    int m_height;
    const std::string m_title;
    bool m_vSync;
    GLFWwindow* m_window;

    //* glfw callbacks
    static void glfwErrorCallback(int error_code, const char* description);
    static void glfwFrameBufferSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwKeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow* window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    //* input managment variables
    static constexpr int MAX_KEYS = 350;
    static constexpr int MAX_BUTTONS = 8;

    std::array<bool, MAX_KEYS> m_keys{};
    std::array<bool, MAX_KEYS> m_keysPressed{};

    std::array<bool, MAX_BUTTONS> m_buttons{};
    std::array<bool, MAX_BUTTONS> m_buttonsPressed{};

    double m_mouseX, m_mouseY;
    double m_dx, m_dy;
    bool m_firstMouse = true;
    bool m_cursorToggle = false;

    bool m_inputEnabled = true;
};