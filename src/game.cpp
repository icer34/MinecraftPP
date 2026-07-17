#include "game.h"

#include <iostream>

#include "game/blocks.h"
#include "util/key_codes.h"

Game::Game()
    : m_window(1600, 900, "MinecraftPP", false),
      m_camera(glm::vec3(0.0f, 90.0f, 3.0f), m_window.getAspectRatio()),
      m_world(World(67)),
      m_renderer(Renderer())
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
        if (m_window.isKeyPressed(Key::W))
        {
            m_camera.move(m_camera.getFront(), m_dt);
        }
        if (m_window.isKeyPressed(Key::A))
        {
            m_camera.move(-m_camera.getRight(), m_dt);
        }
        if (m_window.isKeyPressed(Key::S))
        {
            m_camera.move(-m_camera.getFront(), m_dt);
        }
        if (m_window.isKeyPressed(Key::D))
        {
            m_camera.move(m_camera.getRight(), m_dt);
        }
        if (m_window.isKeyPressed(Key::Space))
        {
            m_camera.move(glm::vec3(0.0f, 1.0f, 0.0f), m_dt);
        }
        if (m_window.isKeyPressed(Key::LCtrl))
        {
            m_camera.move(glm::vec3(0.0f, -1.0f, 0.0f), m_dt);
        }
        if (m_window.consumeKeyPress(Key::F3))
        {
            m_showDebug = !m_showDebug;
        }

        float dx = (float)m_window.consumeDx();
        float dy = (float)m_window.consumeDy();
        m_camera.rotate(dx, dy);
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

    m_world.update(m_camera.getPos(), dt);
}

void Game::render(float dt)
{
    // update fps counter
    m_renderer.updateFPS(dt);

    // render the 3D world (terrain)
    m_renderer.renderWorld(m_world, m_camera);

    // render UI
    m_renderer.beginUI();

    // render debug window if needed
    if (m_showDebug)
        m_renderer.renderDebug(dt);

    if (m_showSettings)
        m_renderer.renderSettings();

    m_renderer.endUI();
}