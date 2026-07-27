#pragma once

#include "game/player.h"
#include "game/world.h"
#include "graphics/hud/hud.h"
#include "graphics/renderer.h"
#include "util/window.h"

class Game
{
public:
    Game();

    void run();

private:
    Window m_window;
    Player m_player;
    World m_world;
    Renderer m_renderer;
    Hud m_hud;

    void processInput();
    void update(float dt);
    void render(float dt);

    float m_dt;
    float m_lastFrameTime;

    bool m_showDebug = true;
    bool m_showSettings = false;
};