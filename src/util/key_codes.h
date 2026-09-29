/**
 * @file key_codes.h
 * @brief Engine-side key and mouse button identifiers, independent of GLFW.
 */

#pragma once

/**
 * @brief Keyboard keys, covering every key GLFW knows.
 *
 * Mapped to GLFW key codes inside Input, so that game code never depends on GLFW directly.
 *
 * Keys are physical: they are named after their position on a US QWERTY keyboard, whatever
 * the layout (Key::W is the Z key of an AZERTY keyboard).
 */
enum class Key
{
    // printable keys
    Space,
    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Semicolon,
    Equal,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LBracket,
    Backslash,
    RBracket,
    GraveAccent,
    World1, ///< non-US key #1
    World2, ///< non-US key #2

    // function keys
    Esc,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,

    // numeric keypad
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadDecimal,
    NumpadDivide,
    NumpadMultiply,
    NumpadSubtract,
    NumpadAdd,
    NumpadEnter,
    NumpadEqual,

    // modifiers
    LShift,
    LCtrl,
    LAlt,
    LSuper,
    RShift,
    RCtrl,
    RAlt,
    RSuper,
    Menu
};

/**
 * @brief Mouse buttons, covering the 8 buttons GLFW knows.
 */
enum class MouseButton
{
    Left,
    Right,
    Middle,
    Button4, ///< usually the "back" side button
    Button5, ///< usually the "forward" side button
    Button6,
    Button7,
    Button8
};
