#pragma once

#include "texture.h"
#include "util/spline.h"
#include <memory>

#include <iostream>

struct World;
struct Shader;
struct Camera;

class Renderer
{
  public:
    Renderer();
    ~Renderer();

    void renderWorld(const World &world, const Camera &cam);
    void renderDebug(float dt);
    void updateFPS(float dt);

    bool requestWorldRegeneration()
    {
        bool result = m_shouldRegenerateWorld;
        m_shouldRegenerateWorld = false;
        return result;
    }

  private:
    std::unique_ptr<Shader> m_shader;
    Texture m_blockTintTexture;

    std::vector<Texture> m_noiseTextures;
    std::vector<Spline *> m_terrainGenSplines;

    float m_fps = 0.0f;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    int m_loadedChunks = 0;
    int m_renderedChunks = 0;

    bool m_shouldRegenerateWorld = false;
};