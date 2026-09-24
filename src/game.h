/**
 * @file game.h
 * @brief Top-level game object: owns every subsystem and runs the main loop.
 */

#pragma once

#include "game/player.h"
#include "game/world.h"
#include "graphics/block_texture_atlas.h"
#include "graphics/hud/hud.h"
#include "graphics/hud/settings_menu.h"
#include "graphics/renderer.h"
#include "util/raycaster.h"
#include "util/window.h"

/**
 * @brief Owns the window, the world, the player and the renderers, and runs the main loop.
 *
 * Members are destroyed in reverse declaration order: everything that holds GL resources
 * is declared after the Window, so it is released while the GL context is still alive.
 */
class Game
{
public:
    /**
     * @brief Opens the window, creates the GL context and all subsystems, and registers the
     * blocks.
     */
    Game();

    /**
     * @brief Runs the main loop (input, update, render, swap) until the window is closed.
     */
    void run();

private:
    Window _window;
    // after _window: holds a GL texture, so it must be destroyed before the GL context
    BlockTextureAtlas _blockAtlas;
    Player _player;
    World _world;

    Renderer _renderer;
    // declared after _window so it is destroyed while the GL context is still alive
    HudRenderer _hudRenderer;
    Hud _hud;
    SettingsMenu _settingsMenu;

    RayCaster _rayCaster;
    RayCastResult _castResult;

    void processInput();
    void update(float dt);
    void render(float dt);

    float _dt;
    float _lastFrameTime;

    bool _showDebug = true;
    bool _showSettings = false;
};