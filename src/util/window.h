/**
 * @file window.h
 * @brief GLFW window, OpenGL context and input handling.
 */

#pragma once

#include <array>
#include <string>

#include "key_codes.h"

#include <glm/glm.hpp>

struct GLFWwindow;

/**
 * @brief Owns the GLFW window and its OpenGL context, and collects keyboard and mouse input.
 *
 * Input is gathered by GLFW callbacks during pollEvents(). Two kinds of queries exist:
 * - `is...Pressed()` reports whether a key / button is currently held down;
 * - `consume...()` reports whether it was pressed since the last consume, then resets it, so
 *   that each press is handled only once.
 *
 * Mouse movement and scrolling accumulate between frames and are reset by their consume
 * methods.
 *
 * Destroying the window destroys the GL context: every object holding GL resources must be
 * destroyed before it.
 */
class Window
{
public:
    /**
     * @brief Initializes GLFW, opens the window, creates an OpenGL 3.3 context, loads the GL
     * functions and initializes ImGui.
     *
     * @param width requested window width, in screen coordinates
     * @param height requested window height, in screen coordinates
     * @param title window title
     * @param vSync whether buffer swaps wait for the vertical sync
     * @throws std::runtime_error if GLFW, the window or the GL loader fails to initialize
     */
    Window(int width, int height, const std::string &title, bool vSync);

    /**
     * @brief Shuts down ImGui, destroys the window and terminates GLFW.
     */
    ~Window();

    /** @brief Processes pending window and input events. Call once per frame. */
    void pollEvents();
    /** @brief Presents the frame that was just rendered. */
    void swapBuffers();
    /** @brief True once the user asked to close the window. */
    bool shouldClose();

    /** @brief Time since GLFW was initialized, in seconds. */
    float getTime() const;
    /** @brief Width / height ratio of the framebuffer. */
    float getAspectRatio() const { return (float)_width / _height; }
    /** @brief Framebuffer width in pixels (can differ from the requested width on HiDPI). */
    int getWidth() const { return _width; }
    /** @brief Framebuffer height in pixels (can differ from the requested height on HiDPI). */
    int getHeight() const { return _height; }

    //* INPUT HANDLING
    /** @brief True while `key` is held down. */
    bool isKeyPressed(Key key) const;
    /** @brief True if `key` was pressed since the last call for that key. */
    bool consumeKeyPress(Key key);

    /** @brief True while `button` is held down. */
    bool isButtonPressed(MouseButton button) const;
    /** @brief True if `button` was pressed since the last call for that button. */
    bool consumeButtonPress(MouseButton button);

    /** @brief Horizontal mouse movement accumulated since the last call, in pixels. */
    double consumeDx();
    /**
     * @brief Vertical mouse movement accumulated since the last call, in pixels (positive when
     * the mouse moves up).
     */
    double consumeDy();
    /** @brief Vertical scroll offset accumulated since the last call. */
    double consumeScroll();
    /** @brief Cursor position in window pixels, from the top-left corner. */
    glm::vec2 getCursorPos();

    /**
     * @brief Discards the accumulated mouse movement, and ignores the movement of the next
     * cursor event (avoids a camera jump when the cursor is captured again).
     */
    void resetMouse();

    /**
     * @brief Shows the cursor (for menus) or hides and captures it (for camera control).
     */
    void setCursorEnabled(bool enabled);
    /** @brief True if the cursor is visible and free. */
    bool isCursorEnabled();

    /** @brief Marks game input as enabled. */
    void enableInput();
    /**
     * @brief Marks game input as disabled and discards the pending key and button presses.
     *
     * Events are still recorded: callers must check isInputEnabled() themselves.
     */
    void disableInput();
    /** @brief True if game input is enabled. */
    bool isInputEnabled();

private:
    int _width;
    int _height;
    const std::string _title;
    bool _vSync;
    GLFWwindow *_window;

    //* glfw callbacks
    static void glfwErrorCallback(int error_code, const char *description);
    static void glfwFrameBufferSizeCallback(GLFWwindow *window, int width, int height);
    static void glfwKeyboardCallback(
        GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset);

    //* input managment variables
    static constexpr int MAX_KEYS = 350;
    static constexpr int MAX_BUTTONS = 8;

    std::array<bool, MAX_KEYS> _keys{};
    std::array<bool, MAX_KEYS> _keysPressed{};

    std::array<bool, MAX_BUTTONS> _buttons{};
    std::array<bool, MAX_BUTTONS> _buttonsPressed{};

    double _mouseX = 0.0, _mouseY = 0.0;
    double _dx = 0.0, _dy = 0.0;
    double _scrollY = 0.0;
    bool _firstMouse = true;
    bool _cursorToggle = false;

    bool _inputEnabled = true;
};