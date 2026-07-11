#include "game.h"

#include <cstdlib>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <iostream>

#include "game/blocks.h"
#include "game/chunk.h"
#include "graphics/texture_atlas.h"
#include "util/key_codes.h"

Game::Game()
    : m_window(1600, 900, "MinecraftPP", false),
      m_camera(glm::vec3(0.0f, 90.0f, 3.0f), m_window.getAspectRatio()),
      m_world(World())
{
    auto &atlas = TextureAtlas::instance();
    atlas.loadAllTextures();

    Blocks::registerAll();
    m_testShader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Game::run()
{
    m_lastFrameTime = m_window.getTime();

    while (!m_window.shouldClose())
    {
        float currentTime = m_window.getTime();
        m_dt = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        updateFpsCounter();

        processInput();

        update(m_dt);

        render();

        m_window.swapBuffers();
    }
}

void Game::processInput()
{
    m_window.pollEvents();

    if (m_window.consumeKeyPress(Key::Esc))
    {
        m_window.toggleCursor();
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
            m_camera.move(m_camera.getUp(), m_dt);
        }
        if (m_window.isKeyPressed(Key::LCtrl))
        {
            m_camera.move(-m_camera.getUp(), m_dt);
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

void Game::update(float dt) { m_world.update(m_camera.getPos(), dt); }

void Game::render()
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_testShader->use();
    m_testShader->setMat4("view", m_camera.getViewMatrix());
    m_testShader->setMat4("projection", m_camera.getProjectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureAtlas::instance().getID());
    m_testShader->setInt("atlas", 0);

    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_testShader->setMat4("model", model);
        mesh->draw();
    }

    m_window.beginImguiFrame();

    ImGui::Begin("Debug");
    ImGui::Text("FPS: %.1f", m_displayedFps);
    ImGui::Text("Frame time: %.2f ms", m_dt * 1000.0f);
    ImGui::End();

    m_window.endImguiFrame();
}

void Game::updateFpsCounter()
{
    m_frameCount++;
    m_fpsTimer += m_dt;

    if (m_fpsTimer >= 1.0f)
    {
        m_displayedFps = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer -= 1.0f;
    }
}
