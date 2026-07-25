#pragma once

#include "texture.h"
#include "util/spline.h"
#include <memory>

#include <iostream>

class World;
class Camera;
class Window;
class CascadedShadowMap;
class Shader;

class Renderer
{
  public:
    Renderer(const Window &window, const World &world);
    ~Renderer();

    void renderWorld(const Camera &cam);

    void beginUI();
    void renderDebug(float dt);
    void renderSettings();
    void endUI();

    void updateFPS(float dt);

    bool requestWorldRegeneration()
    {
        bool result = m_shouldRegenerateWorld;
        m_shouldRegenerateWorld = false;
        return result;
    }

  private:
    const Window &m_window;
    const World &m_world;

    std::unique_ptr<Shader> m_blockShader;
    std::unique_ptr<Shader> m_depthShader;
    std::unique_ptr<CascadedShadowMap> m_shadowMap;

    Texture m_blockTintTexture;

    std::vector<Texture> m_noiseTextures;
    std::vector<Spline *> m_terrainGenSplines;
    int m_loadedChunks = 0;
    int m_renderedChunks = 0;
    float m_pv = 0.0f;
    float m_continentalness = 0.0f;
    float m_erosion = 0.0f;
    glm::vec3 m_lightDir = glm::normalize(glm::vec3(-0.5, -1.0, -0.3));
    bool m_shouldRegenerateWorld = false;

    float m_fps = 0.0f;
    float m_msPerFrame = 0.0f;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;

    glm::vec3 m_camPos;
};