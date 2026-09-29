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
 * Input is gathered by the GLFW callbacks of Window during Window::pollEvents(), once per
 * frame. Two kinds of queries exist:
 * - `is...Pressed()` reports whether a key / button is currently held down;
 * - `was...Pressed()` / `was...Released()` report whether it was pressed / released during
 *   the current frame. They hold for the whole frame, can be read by any number of callers, and
 *   are forgotten at the next pollEvents(): an event nobody read never fires later. A press and
 *   a release within the same frame (quick tap at low FPS) are both reported.
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
    /** @brief True if `key` was pressed during the current frame. */
    bool wasKeyPressed(Key key) const;
    /** @brief True if `key` was released during the current frame. */
    bool wasKeyReleased(Key key) const;

    /** @brief True while `button` is held down. */
    bool isButtonPressed(MouseButton button) const;
    /** @brief True if `button` was pressed during the current frame. */
    bool wasButtonPressed(MouseButton button) const;
    /** @brief True if `button` was released during the current frame. */
    bool wasButtonReleased(MouseButton button) const;

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
     * @brief Marks game input as disabled and discards the key and button presses and releases
     * of the current frame.
     *
     * Events are still recorded: callers must check isInputEnabled() themselves.
     */
    void disableInput();
    /** @brief True if game input is enabled. */
    bool isInputEnabled() const;

private:
    // only Window receives the GLFW events, and forwards them to the handlers below
    friend class Window;

    /// Forgets the presses and releases of the previous frame. Called by Window::pollEvents()
    /// before GLFW delivers the events of the new frame
    void beginFrame();

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
    // pressed / released during the current frame, cleared by beginFrame()
    std::array<bool, MAX_KEYS> _keysPressed{};
    std::array<bool, MAX_KEYS> _keysReleased{};

    std::array<bool, MAX_BUTTONS> _buttons{};
    std::array<bool, MAX_BUTTONS> _buttonsPressed{};
    std::array<bool, MAX_BUTTONS> _buttonsReleased{};

    double _mouseX = 0.0, _mouseY = 0.0;
    double _dx = 0.0, _dy = 0.0;
    double _scrollY = 0.0;
    bool _firstMouse = true;
    bool _cursorToggle = false;

    bool _inputEnabled = true;
};
