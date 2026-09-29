#include "input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace
{
int toGlfwKey(Key key)
{
    switch (key)
    {
    // printable keys
    case Key::Space:
        return GLFW_KEY_SPACE;
    case Key::Apostrophe:
        return GLFW_KEY_APOSTROPHE;
    case Key::Comma:
        return GLFW_KEY_COMMA;
    case Key::Minus:
        return GLFW_KEY_MINUS;
    case Key::Period:
        return GLFW_KEY_PERIOD;
    case Key::Slash:
        return GLFW_KEY_SLASH;
    case Key::Num0:
        return GLFW_KEY_0;
    case Key::Num1:
        return GLFW_KEY_1;
    case Key::Num2:
        return GLFW_KEY_2;
    case Key::Num3:
        return GLFW_KEY_3;
    case Key::Num4:
        return GLFW_KEY_4;
    case Key::Num5:
        return GLFW_KEY_5;
    case Key::Num6:
        return GLFW_KEY_6;
    case Key::Num7:
        return GLFW_KEY_7;
    case Key::Num8:
        return GLFW_KEY_8;
    case Key::Num9:
        return GLFW_KEY_9;
    case Key::Semicolon:
        return GLFW_KEY_SEMICOLON;
    case Key::Equal:
        return GLFW_KEY_EQUAL;
    case Key::A:
        return GLFW_KEY_A;
    case Key::B:
        return GLFW_KEY_B;
    case Key::C:
        return GLFW_KEY_C;
    case Key::D:
        return GLFW_KEY_D;
    case Key::E:
        return GLFW_KEY_E;
    case Key::F:
        return GLFW_KEY_F;
    case Key::G:
        return GLFW_KEY_G;
    case Key::H:
        return GLFW_KEY_H;
    case Key::I:
        return GLFW_KEY_I;
    case Key::J:
        return GLFW_KEY_J;
    case Key::K:
        return GLFW_KEY_K;
    case Key::L:
        return GLFW_KEY_L;
    case Key::M:
        return GLFW_KEY_M;
    case Key::N:
        return GLFW_KEY_N;
    case Key::O:
        return GLFW_KEY_O;
    case Key::P:
        return GLFW_KEY_P;
    case Key::Q:
        return GLFW_KEY_Q;
    case Key::R:
        return GLFW_KEY_R;
    case Key::S:
        return GLFW_KEY_S;
    case Key::T:
        return GLFW_KEY_T;
    case Key::U:
        return GLFW_KEY_U;
    case Key::V:
        return GLFW_KEY_V;
    case Key::W:
        return GLFW_KEY_W;
    case Key::X:
        return GLFW_KEY_X;
    case Key::Y:
        return GLFW_KEY_Y;
    case Key::Z:
        return GLFW_KEY_Z;
    case Key::LBracket:
        return GLFW_KEY_LEFT_BRACKET;
    case Key::Backslash:
        return GLFW_KEY_BACKSLASH;
    case Key::RBracket:
        return GLFW_KEY_RIGHT_BRACKET;
    case Key::GraveAccent:
        return GLFW_KEY_GRAVE_ACCENT;
    case Key::World1:
        return GLFW_KEY_WORLD_1;
    case Key::World2:
        return GLFW_KEY_WORLD_2;

    // function keys
    case Key::Esc:
        return GLFW_KEY_ESCAPE;
    case Key::Enter:
        return GLFW_KEY_ENTER;
    case Key::Tab:
        return GLFW_KEY_TAB;
    case Key::Backspace:
        return GLFW_KEY_BACKSPACE;
    case Key::Insert:
        return GLFW_KEY_INSERT;
    case Key::Delete:
        return GLFW_KEY_DELETE;
    case Key::Right:
        return GLFW_KEY_RIGHT;
    case Key::Left:
        return GLFW_KEY_LEFT;
    case Key::Down:
        return GLFW_KEY_DOWN;
    case Key::Up:
        return GLFW_KEY_UP;
    case Key::PageUp:
        return GLFW_KEY_PAGE_UP;
    case Key::PageDown:
        return GLFW_KEY_PAGE_DOWN;
    case Key::Home:
        return GLFW_KEY_HOME;
    case Key::End:
        return GLFW_KEY_END;
    case Key::CapsLock:
        return GLFW_KEY_CAPS_LOCK;
    case Key::ScrollLock:
        return GLFW_KEY_SCROLL_LOCK;
    case Key::NumLock:
        return GLFW_KEY_NUM_LOCK;
    case Key::PrintScreen:
        return GLFW_KEY_PRINT_SCREEN;
    case Key::Pause:
        return GLFW_KEY_PAUSE;
    case Key::F1:
        return GLFW_KEY_F1;
    case Key::F2:
        return GLFW_KEY_F2;
    case Key::F3:
        return GLFW_KEY_F3;
    case Key::F4:
        return GLFW_KEY_F4;
    case Key::F5:
        return GLFW_KEY_F5;
    case Key::F6:
        return GLFW_KEY_F6;
    case Key::F7:
        return GLFW_KEY_F7;
    case Key::F8:
        return GLFW_KEY_F8;
    case Key::F9:
        return GLFW_KEY_F9;
    case Key::F10:
        return GLFW_KEY_F10;
    case Key::F11:
        return GLFW_KEY_F11;
    case Key::F12:
        return GLFW_KEY_F12;
    case Key::F13:
        return GLFW_KEY_F13;
    case Key::F14:
        return GLFW_KEY_F14;
    case Key::F15:
        return GLFW_KEY_F15;
    case Key::F16:
        return GLFW_KEY_F16;
    case Key::F17:
        return GLFW_KEY_F17;
    case Key::F18:
        return GLFW_KEY_F18;
    case Key::F19:
        return GLFW_KEY_F19;
    case Key::F20:
        return GLFW_KEY_F20;
    case Key::F21:
        return GLFW_KEY_F21;
    case Key::F22:
        return GLFW_KEY_F22;
    case Key::F23:
        return GLFW_KEY_F23;
    case Key::F24:
        return GLFW_KEY_F24;
    case Key::F25:
        return GLFW_KEY_F25;

    // numeric keypad
    case Key::Numpad0:
        return GLFW_KEY_KP_0;
    case Key::Numpad1:
        return GLFW_KEY_KP_1;
    case Key::Numpad2:
        return GLFW_KEY_KP_2;
    case Key::Numpad3:
        return GLFW_KEY_KP_3;
    case Key::Numpad4:
        return GLFW_KEY_KP_4;
    case Key::Numpad5:
        return GLFW_KEY_KP_5;
    case Key::Numpad6:
        return GLFW_KEY_KP_6;
    case Key::Numpad7:
        return GLFW_KEY_KP_7;
    case Key::Numpad8:
        return GLFW_KEY_KP_8;
    case Key::Numpad9:
        return GLFW_KEY_KP_9;
    case Key::NumpadDecimal:
        return GLFW_KEY_KP_DECIMAL;
    case Key::NumpadDivide:
        return GLFW_KEY_KP_DIVIDE;
    case Key::NumpadMultiply:
        return GLFW_KEY_KP_MULTIPLY;
    case Key::NumpadSubtract:
        return GLFW_KEY_KP_SUBTRACT;
    case Key::NumpadAdd:
        return GLFW_KEY_KP_ADD;
    case Key::NumpadEnter:
        return GLFW_KEY_KP_ENTER;
    case Key::NumpadEqual:
        return GLFW_KEY_KP_EQUAL;

    // modifiers
    case Key::LShift:
        return GLFW_KEY_LEFT_SHIFT;
    case Key::LCtrl:
        return GLFW_KEY_LEFT_CONTROL;
    case Key::LAlt:
        return GLFW_KEY_LEFT_ALT;
    case Key::LSuper:
        return GLFW_KEY_LEFT_SUPER;
    case Key::RShift:
        return GLFW_KEY_RIGHT_SHIFT;
    case Key::RCtrl:
        return GLFW_KEY_RIGHT_CONTROL;
    case Key::RAlt:
        return GLFW_KEY_RIGHT_ALT;
    case Key::RSuper:
        return GLFW_KEY_RIGHT_SUPER;
    case Key::Menu:
        return GLFW_KEY_MENU;
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
    case MouseButton::Button4:
        return GLFW_MOUSE_BUTTON_4;
    case MouseButton::Button5:
        return GLFW_MOUSE_BUTTON_5;
    case MouseButton::Button6:
        return GLFW_MOUSE_BUTTON_6;
    case MouseButton::Button7:
        return GLFW_MOUSE_BUTTON_7;
    case MouseButton::Button8:
        return GLFW_MOUSE_BUTTON_8;
    }

    return -1;
}
} // namespace

Input::Input(GLFWwindow *window)
    : _window(window)
{
    static_assert(MAX_KEYS > GLFW_KEY_LAST);
    static_assert(MAX_BUTTONS > GLFW_MOUSE_BUTTON_LAST);

    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

//* ========== EVENT HANDLERS ==========

void Input::beginFrame()
{
    _keysPressed.fill(false);
    _keysReleased.fill(false);
    _buttonsPressed.fill(false);
    _buttonsReleased.fill(false);
}

void Input::onKey(int key, int action)
{
    if (key < 0 || key > GLFW_KEY_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        _keys[key] = true;
        _keysPressed[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        _keys[key] = false;
        _keysReleased[key] = true;
    }
}

void Input::onMouseButton(int button, int action)
{
    if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        _buttons[button] = true;
        _buttonsPressed[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        _buttons[button] = false;
        _buttonsReleased[button] = true;
    }
}

void Input::onCursorPos(double xPos, double yPos)
{
    if (_firstMouse)
    {
        _dx = 0;
        _dy = 0;
        _mouseX = xPos;
        _mouseY = yPos;
        _firstMouse = false;
        return;
    }

    _dx += xPos - _mouseX;
    _dy += _mouseY - yPos;
    _mouseX = xPos;
    _mouseY = yPos;
}

void Input::onScroll(double yOffset) { _scrollY += yOffset; }

//* ======= INPUT HANDLING =========

bool Input::isKeyPressed(Key key) const { return _keys[toGlfwKey(key)]; }

bool Input::wasKeyPressed(Key key) const { return _keysPressed[toGlfwKey(key)]; }

bool Input::wasKeyReleased(Key key) const { return _keysReleased[toGlfwKey(key)]; }

bool Input::isButtonPressed(MouseButton button) const { return _buttons[toGlfwButton(button)]; }

bool Input::wasButtonPressed(MouseButton button) const
{
    return _buttonsPressed[toGlfwButton(button)];
}

bool Input::wasButtonReleased(MouseButton button) const
{
    return _buttonsReleased[toGlfwButton(button)];
}

double Input::consumeDx()
{
    double tmp = _dx;
    _dx = 0.0;
    return tmp;
}

double Input::consumeDy()
{
    double tmp = _dy;
    _dy = 0.0;
    return tmp;
}

double Input::consumeScroll()
{
    double tmp = _scrollY;
    _scrollY = 0.0;
    return tmp;
}

glm::vec2 Input::getCursorPos() const { return glm::vec2(_mouseX, _mouseY); }

void Input::resetMouse()
{
    _firstMouse = true;
    _dx = 0.0;
    _dy = 0.0;
}

void Input::setCursorEnabled(bool enabled)
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

bool Input::isCursorEnabled() const { return _cursorToggle; }

void Input::enableInput() { _inputEnabled = true; }

void Input::disableInput()
{
    _inputEnabled = false;
    _keysPressed.fill(false);
    _keysReleased.fill(false);
    _buttonsPressed.fill(false);
    _buttonsReleased.fill(false);
}

bool Input::isInputEnabled() const { return _inputEnabled; }
