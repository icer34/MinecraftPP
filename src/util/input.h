/**
 * @file input.h
 * @brief Keyboard and mouse input state, fed by the GLFW callbacks of Window.
 */

#pragma once

#include <array>

#include "key_codes.h"

#include <glm/glm.hpp>

struct GLFWwindow;

/**
 * @brief Collects the keyboard and mouse input of a window, and controls its cursor.
 *
 * Input is gathered by the GLFW callbacks of Window during Window::pollEvents(). Two kinds of
 * queries exist:
 * - `is...Pressed()` reports whether a key / button is currently held down;
 * - `consume...()` reports whether it was pressed since the last consume, then resets it, so
 *   that each press is handled only once.
 *
 * Mouse movement and scrolling accumulate between frames and are reset by their consume
 * methods.
 */
class Input
{
public:
    /**
     * @brief Starts with the cursor hidden and captured (see setCursorEnabled()).
     *
     * @param window window whose cursor this object controls; it must outlive this object
     */
    explicit Input(GLFWwindow *window);

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
    glm::vec2 getCursorPos() const;

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
    bool isCursorEnabled() const;

    /** @brief Marks game input as enabled. */
    void enableInput();
    /**
     * @brief Marks game input as disabled and discards the pending key and button presses.
     *
     * Events are still recorded: callers must check isInputEnabled() themselves.
     */
    void disableInput();
    /** @brief True if game input is enabled. */
    bool isInputEnabled() const;

private:
    // only Window receives the GLFW events, and forwards them to the handlers below
    friend class Window;

    //* event handlers, arguments are GLFW values
    void onKey(int key, int action);
    void onMouseButton(int button, int action);
    void onCursorPos(double xPos, double yPos);
    void onScroll(double yOffset);

    GLFWwindow *_window;

    //* input managment variables, indexed by GLFW key / button codes
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
