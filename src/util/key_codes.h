/**
 * @file key_codes.h
 * @brief Engine-side key and mouse button identifiers, independent of GLFW.
 */

#pragma once

/**
 * @brief Keyboard keys the engine listens to.
 *
 * Mapped to GLFW key codes inside Window, so that game code never depends on GLFW directly.
 */
enum class Key
{
    W,
    A,
    S,
    D,
    Space,
    LShift,
    LCtrl,
    Esc,
    F3
};

/**
 * @brief Mouse buttons the engine listens to.
 */
enum class MouseButton
{
    Left,
    Right,
    Middle
};