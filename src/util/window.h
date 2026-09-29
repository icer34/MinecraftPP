/**
 * @file window.h
 * @brief GLFW window and OpenGL context.
 */

#pragma once

#include <string>

#include "input.h"

struct GLFWwindow;

/**
 * @brief Owns the GLFW window and its OpenGL context, and forwards its keyboard and mouse
 * events to an Input (see getInput()).
 *
 * Destroying the window destroys the GL context: every object holding GL resources must be
 * destroyed before it.
 */
class Window
{
public:
    /**
     * @brief Initializes GLFW, opens the window, creates an OpenGL 4.6 core context, loads the
     * GL functions and initializes ImGui.
     *
     * The context uses a [0, 1] NDC depth range (`glClipControl`), and in Debug builds, the GL
     * debug output is enabled and printed to stderr (see gl::onDebugMessage()). The default
     * framebuffer is not multisampled: the Renderer draws the scene into its own framebuffer.
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

    /** @brief Keyboard and mouse input of this window, updated by pollEvents(). */
    Input &getInput() { return _input; }
    /** @brief Keyboard and mouse input of this window, updated by pollEvents(). */
    const Input &getInput() const { return _input; }

private:
    int _width;
    int _height;
    const std::string _title;
    bool _vSync;
    GLFWwindow *_window;
    // after _window: built from it
    Input _input;

    /**
     * @brief Initializes GLFW and opens the window (without making its context current).
     *
     * @throws std::runtime_error if GLFW or the window fails to initialize
     */
    static GLFWwindow *createGlfwWindow(int width, int height, const std::string &title);

    //* glfw callbacks
    static void glfwErrorCallback(int error_code, const char *description);
    static void glfwFrameBufferSizeCallback(GLFWwindow *window, int width, int height);
    static void glfwKeyboardCallback(
        GLFWwindow *window, int key, int scancode, int action, int mods);
    static void glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
    static void glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset);
};
