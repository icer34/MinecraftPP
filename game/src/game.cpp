#include "game.h"

#include <iostream>

#include "blocks.h"
#include "util/key_codes.h"

using glm::vec3;

Game::Game()
    : m_player(vec3(-96.0f, 110.0f, 30.2f)),
      m_world(World(67))
{
}

void Game::init(Window &window, Renderer &renderer)
{
    m_renderer = &renderer;

    Blocks::registerAll();

    m_hud = std::make_unique<Hud>(window.getWidth(), window.getHeight());
    m_settingsMenu = std::make_unique<SettingsMenu>();
    m_rayCaster = std::make_unique<RayCaster>(m_world);
}

void Game::processInput(Window &window)
{
    if (window.consumeKeyPress(Key::Esc))
    {
        m_showSettings = !m_showSettings;
        window.setCursorEnabled(m_showSettings);
        if (m_showSettings)
            m_settingsMenu->resetNavigation();
    }

    if (!window.isCursorEnabled())
    {
        InputData inputData;

        if (window.consumeButtonPress(MouseButton::Left))
        {
            m_world.breakBlock(m_castResult.targetPos);
        }
        if (window.consumeButtonPress(MouseButton::Right))
        {
            glm::vec3 placePos = m_castResult.targetPos + m_castResult.targetNorm;
            glm::vec3 blockCenter = glm::floor(placePos) + glm::vec3(0.5f, 0.0f, 0.5f);
            AABB blockBox(glm::vec3(0.5f));
            if (!blockBox.intersects(m_player.getHitBox(), blockCenter, m_player.getPos()))
                m_world.placeBlock(Blocks::STONE, placePos);
        }
        if (window.isKeyPressed(Key::W))
        {
            inputData.move += m_player.getFront();
        }
        if (window.isKeyPressed(Key::A))
        {
            inputData.move -= m_player.getRight();
        }
        if (window.isKeyPressed(Key::S))
        {
            inputData.move -= m_player.getFront();
        }
        if (window.isKeyPressed(Key::D))
        {
            inputData.move += m_player.getRight();
        }
        if (window.isKeyPressed(Key::Space))
        {
            inputData.jump = true;
        }
        if (window.consumeKeyPress(Key::F3))
        {
            m_showDebug = !m_showDebug;
        }

        inputData.move.y = 0.0f;
        if (glm::length(inputData.move) > 0.0f)
            inputData.move = glm::normalize(inputData.move);

        inputData.mouseDx = (float)window.consumeDx();
        inputData.mouseDy = (float)window.consumeDy();
        inputData.scroll = (float)window.consumeScroll();

        m_player.consumeInput(inputData);
    }
    else
    {
        window.consumeDx();
        window.consumeDy();
    }
}

void Game::update(float dt)
{
    if (m_renderer->requestWorldRegeneration())
    {
        m_world.regenerate();
    }

    m_player.update(dt, m_world);

    m_castResult = m_rayCaster->cast(
        m_player.getCam().getPos(), m_player.getCam().getFront(), m_player.getReach());

    m_world.update(m_player.getPos(), dt);
}

void Game::render(float dt, Window &window)
{
    // update fps counter
    m_renderer->updateFPS(dt);

    // render the 3D world (terrain)
    m_renderer->renderWorld(m_player.getCam());
    if (m_castResult.hit)
        m_renderer->renderBlockOutline(m_castResult, m_player.getCam());

    m_hud->render();

    // render UI
    m_renderer->beginUI();

    // render debug window if needed
    if (m_showDebug)
        m_renderer->renderDebug(dt);

    if (m_showSettings)
    {
        bool closeRequested = m_settingsMenu->render(window.getWidth(),
                                                     window.getHeight(),
                                                     window.getCursorPos(),
                                                     window.consumeButtonPress(MouseButton::Left),
                                                     window.isButtonPressed(MouseButton::Left),
                                                     window.consumeScroll());
        if (closeRequested)
        {
            window.setCursorEnabled(false);
            m_showSettings = false;
        }
    }

    m_renderer->endUI();
}
