#include "game.h"

#include <iostream>

#include "debug/debug_settings.h"
#include "game/blocks.h"
#include "util/input.h"

using glm::vec3;

Game::Game()
    : _window(1600, 900, "MinecraftPP", false),
      _player(vec3(-96.0f, 110.0f, 30.2f)),
      _world(World(67)),
      _renderer(Renderer(_window, _world, _blockAtlas)),
      _hud(_hudRenderer),
      _settingsMenu(_hudRenderer),
      _rayCaster(_world)
{
    // the block textures must be loaded before registering the blocks that refer to them
    _blockAtlas.loadAllTextures();
    Blocks::registerAll(_blockAtlas);
}

void Game::run()
{
    _lastFrameTime = _window.getTime();

    while (!_window.shouldClose())
    {
        double frameStart = _window.getTime();
        _dt = (float)(frameStart - _lastFrameTime);
        _lastFrameTime = frameStart;

        processInput();

        update(_dt);

        render(_dt);

        // measured before the swap, which is where the vsync wait mostly happens
        _cpuTime = (float)(_window.getTime() - frameStart);

        _window.swapBuffers();
    }
}

void Game::processInput()
{
    _window.pollEvents();
    Input &input = _window.getInput();

    _debugUI.handleInput(input);

    if (input.wasKeyPressed(Key::Esc))
    {
        _showSettings = !_showSettings;
        input.setCursorEnabled(_showSettings);
        if (_showSettings)
            _settingsMenu.resetNavigation();
    }

    if (!input.isCursorEnabled())
    {
        InputData inputData;

        if (input.wasButtonPressed(MouseButton::Left))
        {
            _world.breakBlock(_castResult.targetPos);
        }
        if (input.wasButtonPressed(MouseButton::Right))
        {
            glm::vec3 placePos = _castResult.targetPos + _castResult.targetNorm;
            glm::vec3 blockCenter = glm::floor(placePos) + glm::vec3(0.5f, 0.0f, 0.5f);
            AABB blockBox(glm::vec3(0.5f));
            if (!blockBox.intersects(_player.getHitBox(), blockCenter, _player.getPos()))
                _world.placeBlock(Blocks::STONE, placePos);
        }
        if (input.isKeyPressed(Key::W))
        {
            inputData.move += _player.getFront();
        }
        if (input.isKeyPressed(Key::A))
        {
            inputData.move -= _player.getRight();
        }
        if (input.isKeyPressed(Key::S))
        {
            inputData.move -= _player.getFront();
        }
        if (input.isKeyPressed(Key::D))
        {
            inputData.move += _player.getRight();
        }
        if (input.isKeyPressed(Key::Space))
        {
            inputData.jump = true;
        }

        inputData.move.y = 0.0f;
        if (glm::length(inputData.move) > 0.0f)
            inputData.move = glm::normalize(inputData.move);

        inputData.mouseDx = (float)input.consumeDx();
        inputData.mouseDy = (float)input.consumeDy();
        inputData.scroll = (float)input.consumeScroll();

        _player.consumeInput(inputData);
    }
    else
    {
        input.consumeDx();
        input.consumeDy();
    }
}

void Game::update(float dt)
{
    if (_renderer.requestWorldRegeneration())
    {
        _world.regenerate();
    }

    _player.update(dt, _world);

    _castResult = _rayCaster.cast(
        _player.getCam().getPos(), _player.getCam().getFront(), _player.getReach());

    _world.update(_player.getPos(), dt);
}

void Game::render(float dt)
{
    _gpuTimer.begin();

    // recorded every frame, even when the panel is hidden, to keep the timing averages going
    glm::vec3 pos = _player.getCam().getPos();
    auto &terrain = TerrainGenerator::instance();

    DebugFrameInfo info;
    info.frameTime = dt;
    info.cpuTime = _cpuTime;
    info.gpuTime = _gpuTimer.getLastTime();
    info.cameraPos = pos;
    info.loadedChunks = _world.getChunkCount();
    info.renderedChunks = _renderer.getRenderedChunkCount();
    info.pvNoise = terrain.getPvNoise().sample(pos.x, pos.z);
    info.erosionNoise = terrain.getErosionNoise().sample(pos.x, pos.z);
    info.continentalnessNoise = terrain.getContinentalnessNoise().sample(pos.x, pos.z);

    _debugUI.recordFrame(info);

    // render the 3D world (terrain) into the scene framebuffer, then show it on screen
    _renderer.renderWorld(_player.getCam());
    if (_castResult.hit)
        _renderer.renderBlockOutline(_castResult);
    _renderer.presentScene();

    // everything below is drawn directly on the screen

    _hud.render(_window.getWidth(), _window.getHeight());

    // render UI
    _renderer.beginUI();

    // render debug window if needed
    if (debugSettings().showPanel)
        _debugUI.render();

    if (_showSettings)
    {
        Input &input = _window.getInput();
        bool closeRequested = _settingsMenu.render(_window.getWidth(),
                                                   _window.getHeight(),
                                                   input.getCursorPos(),
                                                   input.wasButtonPressed(MouseButton::Left),
                                                   input.isButtonPressed(MouseButton::Left),
                                                   input.consumeScroll());
        if (closeRequested)
        {
            input.setCursorEnabled(false);
            _showSettings = false;
        }
    }

    _renderer.endUI();

    _gpuTimer.end();
}