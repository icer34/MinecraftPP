/**
 * @file renderer.h
 * @brief 3D world renderer and ImGui debug overlay.
 */

#pragma once

#include "mesh/block_outline.h"
#include "texture.h"
#include "util/spline.h"
#include <memory>

#include <iostream>

class World;
class Camera;
class Window;
class CascadedShadowMap;
class Shader;
class FrameBuffer;
struct RayCastResult;

/**
 * @brief Draws the 3D world and the ImGui debug UI.
 *
 * A world frame is made of several passes, in this order:
 * 1. shadow pass: all chunk meshes into the cascaded shadow map;
 * 2. opaque pass: frustum-culled chunk meshes, with shadows;
 * 3. copy of the opaque color and depth into an off-screen framebuffer;
 * 4. water pass, which samples that copy for refraction;
 * 5. sky, drawn as a fullscreen triangle behind everything.
 *
 * Must be constructed after the GL context exists (i.e. after the Window).
 */
class Renderer
{
public:
    /**
     * @brief Loads the shaders and textures, fills the block texture atlas and sets up the
     * global GL state.
     *
     * @param window window to render into, must outlive the renderer
     * @param world world to render, must outlive the renderer
     */
    Renderer(const Window &window, const World &world);
    ~Renderer();

    /**
     * @brief Renders one frame of the 3D world (shadows, terrain, water, sky).
     *
     * @param cam camera to render from; its aspect ratio is updated to match the window
     */
    void renderWorld(Camera &cam);

    /**
     * @brief Starts a new ImGui frame. Call before any ImGui widget of the frame.
     */
    void beginUI();

    /**
     * @brief Draws the debug panel (FPS, position, chunk counts, terrain noise values).
     *
     * Must be called between beginUI() and endUI().
     */
    void renderDebug();

    /**
     * @brief Renders the ImGui draw data of the current frame.
     */
    void endUI();

    /**
     * @brief Draws the outline of the block targeted by a raycast.
     *
     * @param result raycast result; only meaningful when `result.hit` is true
     * @param cam camera to render from
     */
    void renderBlockOutline(const RayCastResult &result, const Camera &cam);

    /**
     * @brief Updates the FPS counter shown in the debug panel. Call once per frame.
     *
     * @param dt duration of the last frame, in seconds
     */
    void updateFPS(float dt);

    /**
     * @brief Returns true once if a world regeneration was requested from the UI, then
     * resets the request.
     */
    bool requestWorldRegeneration();

private:
    const Window &_window;
    const World &_world;
    BlockOutline _blockOutline;

    std::unique_ptr<Shader> _blockShader;
    std::unique_ptr<Shader> _depthShader;
    std::unique_ptr<Shader> _waterShader;
    std::unique_ptr<Shader> _skyShader;
    std::unique_ptr<CascadedShadowMap> _shadowMap;
    std::unique_ptr<FrameBuffer> _frameBuffer;

    Texture _blockTintTexture;

    unsigned int _skyVAO;

    int _loadedChunks = 0;
    int _renderedChunks = 0;
    glm::vec3 _lightDir = glm::normalize(glm::vec3(-0.8, -0.3, -0.6));
    bool _shouldRegenerateWorld = false;

    float _fps = 0.0f;
    float _msPerFrame = 0.0f;
    int _frameCount = 0;
    float _fpsTimer = 0.0f;

    glm::vec3 _camPos;
};