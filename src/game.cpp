#include "game.h"

#include <iostream>

#include "game/blocks.h"
#include "util/key_codes.h"

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
        float currentTime = _window.getTime();
        _dt = currentTime - _lastFrameTime;
        _lastFrameTime = currentTime;

        processInput();

        update(_dt);

        render(_dt);

        _window.swapBuffers();
    }
}

void Game::processInput()
{
    _window.pollEvents();

    if (_window.consumeKeyPress(Key::Esc))
    {
        _showSettings = !_showSettings;
        _window.setCursorEnabled(_showSettings);
        if (_showSettings)
            _settingsMenu.resetNavigation();
    }

    if (!_window.isCursorEnabled())
    {
        InputData inputData;

        if (_window.consumeButtonPress(MouseButton::Left))
        {
            _world.breakBlock(_castResult.targetPos);
        }
        if (_window.consumeButtonPress(MouseButton::Right))
        {
            glm::vec3 placePos = _castResult.targetPos + _castResult.targetNorm;
            glm::vec3 blockCenter = glm::floor(placePos) + glm::vec3(0.5f, 0.0f, 0.5f);
            AABB blockBox(glm::vec3(0.5f));
            if (!blockBox.intersects(_player.getHitBox(), blockCenter, _player.getPos()))
                _world.placeBlock(Blocks::STONE, placePos);
        }
        if (_window.isKeyPressed(Key::W))
        {
            inputData.move += _player.getFront();
        }
        if (_window.isKeyPressed(Key::A))
        {
            inputData.move -= _player.getRight();
        }
        if (_window.isKeyPressed(Key::S))
        {
            inputData.move -= _player.getFront();
        }
        if (_window.isKeyPressed(Key::D))
        {
            inputData.move += _player.getRight();
        }
        if (_window.isKeyPressed(Key::Space))
        {
            inputData.jump = true;
        }
        if (_window.consumeKeyPress(Key::F3))
        {
            _showDebug = !_showDebug;
        }

        inputData.move.y = 0.0f;
        if (glm::length(inputData.move) > 0.0f)
            inputData.move = glm::normalize(inputData.move);

        inputData.mouseDx = (float)_window.consumeDx();
        inputData.mouseDy = (float)_window.consumeDy();
        inputData.scroll = (float)_window.consumeScroll();

        _player.consumeInput(inputData);
    }
    else
    {
        _window.consumeDx();
        _window.consumeDy();
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
    // update fps counter
    _renderer.updateFPS(dt);

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
    if (_showDebug)
        _renderer.renderDebug();

    if (_showSettings)
    {
        bool closeRequested = _settingsMenu.render(_window.getWidth(),
                                                   _window.getHeight(),
                                                   _window.getCursorPos(),
                                                   _window.consumeButtonPress(MouseButton::Left),
                                                   _window.isButtonPressed(MouseButton::Left),
                                                   _window.consumeScroll());
        if (closeRequested)
        {
            _window.setCursorEnabled(false);
            _showSettings = false;
        }
    }

    _renderer.endUI();
}