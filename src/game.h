#pragma once

#include "game/chunk.h"
#include "game/world.h"
#include "graphics/camera.h"
#include "graphics/shader.h"
#include "util/window.h"

#include <memory>

class Game
{
  public:
    Game();

    void run();

  private:
    Window m_window;
    Camera m_camera;
    World m_world;

    std::unique_ptr<Shader> m_testShader;

    void processInput();
    void update(float dt);
    void render();

    float m_dt;
    float m_lastFrameTime;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    float m_displayedFps = 0.0f;

    void updateFpsCounter();
};