#include "game.h"

#include <iostream>

#include "game/blocks.h"
#include "util/key_codes.h"

using glm::vec3;

Game::Game()
    : m_window(1600, 900, "MinecraftPP", false),
      m_player(vec3(-96.0f, 110.0f, 30.2f)),
      m_world(World(67)),
      m_renderer(Renderer(m_window, m_world)),
      m_hud(m_window.getWidth(), m_window.getHeight()),
      m_rayCaster(m_world)
{
    // register all blocks
    Blocks::registerAll();
}

void Game::run()
{
    m_lastFrameTime = m_window.getTime();

    while (!m_window.shouldClose())
    {
        float currentTime = m_window.getTime();
        m_dt = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        processInput();

        update(m_dt);

        render(m_dt);

        m_window.swapBuffers();
    }
}

void Game::processInput()
{
    m_window.pollEvents();

    if (m_window.consumeKeyPress(Key::Esc))
    {
        m_showSettings = !m_showSettings;
        m_window.setCursorEnabled(m_showSettings);
        if (m_showSettings)
            m_settingsMenu.resetNavigation();
    }

    if (!m_window.isCursorEnabled())
    {
        InputData inputData;

        if (m_window.consumeButtonPress(MouseButton::Left))
        {
            m_world.breakBlock(m_castResult.targetPos);
        }
        if (m_window.consumeButtonPress(MouseButton::Right))
        {
            glm::vec3 placePos = m_castResult.targetPos + m_castResult.targetNorm;
            glm::vec3 blockCenter = glm::floor(placePos) + glm::vec3(0.5f, 0.0f, 0.5f);
            AABB blockBox(glm::vec3(0.5f));
            if (!blockBox.intersects(m_player.getHitBox(), blockCenter, m_player.getPos()))
                m_world.placeBlock(Blocks::STONE, placePos);
        }
        if (m_window.isKeyPressed(Key::W))
        {
            inputData.move += m_player.getFront();
        }
        if (m_window.isKeyPressed(Key::A))
        {
            inputData.move -= m_player.getRight();
        }
        if (m_window.isKeyPressed(Key::S))
        {
            inputData.move -= m_player.getFront();
        }
        if (m_window.isKeyPressed(Key::D))
        {
            inputData.move += m_player.getRight();
        }
        if (m_window.isKeyPressed(Key::Space))
        {
            inputData.jump = true;
        }
        if (m_window.consumeKeyPress(Key::F3))
        {
            m_showDebug = !m_showDebug;
        }

        inputData.move.y = 0.0f;
        if (glm::length(inputData.move) > 0.0f)
            inputData.move = glm::normalize(inputData.move);

        inputData.mouseDx = (float)m_window.consumeDx();
        inputData.mouseDy = (float)m_window.consumeDy();
        inputData.scroll = (float)m_window.consumeScroll();

        m_player.consumeInput(inputData);
    }
    else
    {
        m_window.consumeDx();
        m_window.consumeDy();
    }
}

void Game::update(float dt)
{
    if (m_renderer.requestWorldRegeneration())
    {
        m_world.regenerate();
    }

    m_player.update(dt, m_world);

    m_castResult = m_rayCaster.cast(
        m_player.getCam().getPos(), m_player.getCam().getFront(), m_player.getReach());

    m_world.update(m_player.getPos(), dt);
}

void Game::render(float dt)
{
    // update fps counter
    m_renderer.updateFPS(dt);

    // render the 3D world (terrain)
    m_renderer.renderWorld(m_player.getCam());
    if (m_castResult.hit)
        m_renderer.renderBlockOutline(m_castResult, m_player.getCam());

    m_hud.render();

    // render UI
    m_renderer.beginUI();

    // render debug window if needed
    if (m_showDebug)
        m_renderer.renderDebug(dt);

    if (m_showSettings)
    {
        bool closeRequested = m_settingsMenu.render(m_window.getWidth(),
                                                    m_window.getHeight(),
                                                    m_window.getCursorPos(),
                                                    m_window.consumeButtonPress(MouseButton::Left),
                                                    m_window.isButtonPressed(MouseButton::Left),
                                                    m_window.consumeScroll());
        if (closeRequested)
        {
            m_window.setCursorEnabled(false);
            m_showSettings = false;
        }
    }

    m_renderer.endUI();
}