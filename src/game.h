/**
 * @file game.h
 * @brief Top-level game object: owns every subsystem and runs the main loop.
 */

#pragma once

#include "debug/debug_draw.h"
#include "debug/debug_ui.h"
#include "game/entity/player.h"
#include "game/world/world.h"
#include "graphics/block_texture_atlas.h"
#include "graphics/gl/gpu_timer.h"
#include "graphics/hud/hud.h"
#include "graphics/hud/settings_menu.h"
#include "graphics/renderer.h"
#include "game/world/raycaster.h"
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
    GpuTimer _gpuTimer;
    HudRenderer _hudRenderer;
    Hud _hud;
    SettingsMenu _settingsMenu;

    DebugUI _debugUI;
    DebugDraw _debugDraw;

    RayCaster _rayCaster;
    RayCastResult _castResult;

    void processInput();
    void update(float dt);
    void render(float dt);

    float _dt;
    double _lastFrameTime;
    float _cpuTime = 0.0f; // of the previous frame, see DebugFrameInfo::cpuTime

    bool _showSettings = false;
};