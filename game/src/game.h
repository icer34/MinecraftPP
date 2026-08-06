#pragma once

#include <memory>

#include "app/application_interface.h"
#include "player.h"
#include "world.h"
#include "graphics/hud/hud.h"
#include "graphics/hud/settings_menu.h"
#include "graphics/renderer.h"
#include "util/raycaster.h"
#include "util/window.h"

class Game : public IApplication
{
public:
    Game();

    IVoxelWorld &getWorld() override { return m_world; }
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
