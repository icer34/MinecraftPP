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

class Renderer
{
public:
    Renderer(const Window &window, const World &world);
    ~Renderer();

    void renderWorld(Camera &cam);

    void beginUI();
    void renderDebug(float dt);
    void endUI();

    void renderBlockOutline(const RayCastResult &result, const Camera &cam);

    void updateFPS(float dt);

    bool requestWorldRegeneration();

private:
    const Window &m_window;
    const World &m_world;
    BlockOutline m_blockOutline;

    std::unique_ptr<Shader> m_blockShader;
    std::unique_ptr<Shader> m_depthShader;
    std::unique_ptr<Shader> m_waterShader;
    std::unique_ptr<CascadedShadowMap> m_shadowMap;
    std::unique_ptr<FrameBuffer> m_frameBuffer;

    Texture m_blockTintTexture;

    int m_loadedChunks = 0;
    int m_renderedChunks = 0;
    glm::vec3 m_lightDir = glm::normalize(glm::vec3(-0.3, -0.6, -0.3));
    bool m_shouldRegenerateWorld = false;

    float m_fps = 0.0f;
    float m_msPerFrame = 0.0f;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;

    glm::vec3 m_camPos;
};