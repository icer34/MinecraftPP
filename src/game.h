#pragma once

#include "game/world.h"
#include "graphics/camera.h"
#include "graphics/renderer.h"
#include "util/window.h"

class Game
{
  public:
    Game();

    void run();

  private:
    Window m_window;
    Camera m_camera;
    World m_world;
    Renderer m_renderer;

    void processInput();
    void update(float dt);
    void render(float dt);

    float m_dt;
    float m_lastFrameTime;

    bool m_showDebug = true;
    bool m_showSettings = false;
};