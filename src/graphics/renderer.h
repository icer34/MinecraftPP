/**
 * @file renderer.h
 * @brief 3D world renderer and ImGui debug overlay.
 */

#pragma once

#include "gl/gl_objects.h"
#include "mesh/block_outline.h"
#include "texture.h"
#include "util/spline.h"
#include <memory>
#include <vector>

#include <iostream>

class World;
class Camera;
class Window;
class CascadedShadowMap;
class Shader;
class FrameBuffer;
class FrameDataBuffer;
class ChunkMesh;
struct RayCastResult;
class BlockTextureAtlas;

/**
 * @brief Draws the 3D world and the ImGui debug UI.
 *
 * The world is not drawn to the screen directly, but into a multisampled HDR scene
 * framebuffer (`GL_RGBA16F` color, `GL_DEPTH_COMPONENT32F` reverse-Z depth). A frame is made
 * of several passes, in this order:
 * 1. renderWorld():
 *    1. upload of the FrameData uniform buffer, shared by every shader;
 *    2. shadow pass: all chunk meshes into the cascaded shadow map;
 *    3. opaque pass: frustum-culled chunk meshes, with shadows;
 *    4. copy (resolve) of the opaque color and depth into a single-sampled framebuffer;
 *    5. water pass, which samples that copy for refraction and reflections;
 *    6. sky, drawn as a fullscreen triangle behind everything;
 * 2. renderBlockOutline(), optional;
 * 3. presentScene(): resolves the scene and copies it to the screen;
 * 4. then the HUD and ImGui, drawn directly on the screen.
 *
 * Must be constructed after the GL context exists (i.e. after the Window).
 */
class Renderer
{
public:
    /**
     * @brief Loads the shaders and textures and sets up the global GL state.
     *
     * @param window window to render into, must outlive the renderer
     * @param world world to render, must outlive the renderer
     * @param blockAtlas block texture atlas, must outlive the renderer
     */
    Renderer(const Window &window, const World &world, const BlockTextureAtlas &blockAtlas);
    ~Renderer();

    /**
     * @brief Renders one frame of the 3D world (shadows, terrain, water, sky) into the scene
     * framebuffer. Call presentScene() to show it.
     *
     * @param cam camera to render from; its aspect ratio is updated to match the window
     */
    void renderWorld(Camera &cam);

    /**
     * @brief Draws the outline of the block targeted by a raycast into the scene framebuffer.
     *
     * Call between renderWorld() and presentScene().
     *
     * @param result raycast result; only meaningful when `result.hit` is true
     */
    void renderBlockOutline(const RayCastResult &result);

    /**
     * @brief Resolves the scene framebuffer and copies it to the screen. Leaves the default
     * framebuffer bound, for the HUD and ImGui.
     */
    void presentScene();

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
    // (re)creates the scene framebuffers if they don't match the window size
    void updateFramebufferSize();
    void uploadFrameData(const Camera &cam);

    const Window &_window;
    const World &_world;
    const BlockTextureAtlas &_blockAtlas;
    BlockOutline _blockOutline;

    std::unique_ptr<Shader> _blockShader;
    std::unique_ptr<Shader> _depthShader;
    std::unique_ptr<Shader> _waterShader;
    std::unique_ptr<Shader> _skyShader;
    std::unique_ptr<CascadedShadowMap> _shadowMap;
    std::unique_ptr<FrameDataBuffer> _frameData;

    // multisampled HDR framebuffer the world is rendered into
    std::unique_ptr<FrameBuffer> _sceneFbo;
    // single-sampled copy of the scene: sampled by the water pass, then used to present
    std::unique_ptr<FrameBuffer> _resolvedFbo;

    Texture _blockTintTexture;

    GLVertexArray _chunkVao; // packed vertex format shared by every chunk mesh
    GLVertexArray _skyVao;   // empty: the sky triangle is generated from gl_VertexID

    // chunk meshes inside the view frustum, refilled every frame (kept to reuse its memory)
    std::vector<ChunkMesh *> _visibleMeshes;

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
