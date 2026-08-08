#pragma once

#include <memory>

#include "core/world.h"
#include "engine.h"
#include "player.h"

class Game : public IApplication
{
public:
    Game();

    World &getWorld() override { return m_world; }
    void init(Window &window, Renderer &renderer) override;
    void processInput(Window &window) override;
    void update(float dt) override;
    void render(float dt, Window &window) override;

private:
    Player m_player;
    World m_world;

    Renderer *m_renderer = nullptr;
    std::unique_ptr<Hud> m_hud;
    std::unique_ptr<SettingsMenu> m_settingsMenu;

    std::unique_ptr<RayCaster> m_rayCaster;
    RayCastResult m_castResult;

    bool m_showDebug = true;
    bool m_showSettings = false;
};
