#include "game.h"

#include <iostream>

#include "game/blocks.h"
#include "util/key_codes.h"

using glm::vec3;

Game::Game()
    : m_window(1600, 900, "MinecraftPP", false),
      m_player(vec3(0.5f, 110.0f, 3.2f)),
      m_world(World(67)),
      m_renderer(Renderer(m_window, m_world)),
      m_hud(m_window.getWidth(), m_window.getHeight())
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
        m_window.toggleCursor();
        m_showSettings = !m_showSettings;
    }

    if (!m_window.isCursorEnabled())
    {
        vec3 moveInput{0.0f};
        bool jumpPressed = false;

        if (m_window.isKeyPressed(Key::W))
        {
            moveInput += m_player.getFront();
        }
        if (m_window.isKeyPressed(Key::A))
        {
            moveInput -= m_player.getRight();
        }
        if (m_window.isKeyPressed(Key::S))
        {
            moveInput -= m_player.getFront();
        }
        if (m_window.isKeyPressed(Key::D))
        {
            moveInput += m_player.getRight();
        }
        if (m_window.isKeyPressed(Key::Space))
        {
            jumpPressed = true;
        }
        if (m_window.consumeKeyPress(Key::F3))
        {
            m_showDebug = !m_showDebug;
        }

        moveInput.y = 0.0f;
        if (glm::length(moveInput) > 0.0f)
            moveInput = glm::normalize(moveInput);
        m_player.setMoveInput(moveInput, jumpPressed);

        float dx = (float)m_window.consumeDx();
        float dy = (float)m_window.consumeDy();
        m_player.rotateCam(dx, dy);

        float scroll = (float)m_window.consumeScroll();
        if (scroll != 0.0f)
        {
            m_player.zoomCam(scroll);
        }
    }
    else
    {
        m_window.consumeDx();
        m_window.consumeDy();
        m_window.consumeScroll();
    }
}

void Game::update(float dt)
{
    if (m_renderer.requestWorldRegeneration())
    {
        m_world.regenerate();
    }

    m_player.update(dt, m_world);

    m_world.update(m_player.getPos(), dt);
}

void Game::render(float dt)
{
    // update fps counter
    m_renderer.updateFPS(dt);

    // render the 3D world (terrain)
    m_renderer.renderWorld(m_player.getCam());

    // render UI
    m_renderer.beginUI();

    // render debug window if needed
    if (m_showDebug)
        m_renderer.renderDebug(dt);

    if (m_showSettings)
        m_renderer.renderSettings();

    m_renderer.endUI();

    m_hud.render();
}