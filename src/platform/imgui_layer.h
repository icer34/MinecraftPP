/**
 * @file imgui_layer.h
 * @brief Dear ImGui and ImPlot setup, and the begin / end of each ImGui frame.
 */

#pragma once

class Window;

/**
 * @brief Owns the ImGui and ImPlot contexts of a Window, and starts and renders each ImGui
 * frame.
 *
 * Everything that draws ImGui widgets (DebugUI, SettingsMenu) must do it between beginFrame()
 * and endFrame().
 *
 * Must be constructed after the Window and destroyed before it: its GLFW backend installs its
 * callbacks after the Window's ones, and chains to them; its OpenGL backend needs the GL
 * context to release its objects.
 */
class ImGuiLayer
{
public:
    /**
     * @brief Creates the ImGui and ImPlot contexts and initializes the GLFW and OpenGL
     * backends.
     *
     * @param window window whose events ImGui receives, must outlive this object
     */
    explicit ImGuiLayer(const Window &window);

    /**
     * @brief Shuts down the backends and destroys the contexts.
     */
    ~ImGuiLayer();

    // owns global ImGui state: exactly one instance
    ImGuiLayer(const ImGuiLayer &) = delete;
    ImGuiLayer &operator=(const ImGuiLayer &) = delete;

    /**
     * @brief Starts a new ImGui frame. Call before any ImGui widget of the frame.
     */
    void beginFrame();

    /**
     * @brief Renders the ImGui draw data of the current frame into the bound framebuffer.
     */
    void endFrame();
};
