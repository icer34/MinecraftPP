#include "game.h"

#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <cstdlib>

#include "game/blocks.h"
#include "game/chunk.h"
#include "util/key_codes.h"
#include "graphics/texture_atlas.h"

Game::Game() 
    : m_window(1600, 900, "MinecraftPP", true)
    , m_camera(glm::vec3(0.0f, 0.0f, 3.0f), m_window.getAspectRatio())
{
    auto& atlas = TextureAtlas::instance();
    atlas.loadAllTextures();
    
    Blocks::registerAll();
    m_testShader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");

    //setup test chunk
    m_testChunk = std::make_unique<Chunk>(ChunkCoord{0, 0, 0});
    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int z = 0; z < Chunk::SIZE; z++)
        {
            for (int y = 0; y < Chunk::SIZE; y++)
            {
                m_testChunk->setBlock(Blocks::GRASS, x, y, z);
            }
        }
    }

    //create the chunk mesh
    m_chunkMesh = std::make_unique<ChunkMesh>();
    ChunkMesher chunkMesher;

    std::array<const Chunk*, 6> neighbors0;
    neighbors0.fill(nullptr);
    chunkMesher.mesh(*m_testChunk, neighbors0, *m_chunkMesh);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void Game::run() 
{
    m_lastFrameTime = m_window.getTime();
   
    while(!m_window.shouldClose())
    {
        float currentTime = m_window.getTime();
        m_dt = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        updateFpsCounter();

        processInput();

        update();

        render();

        m_window.swapBuffers();
    }
}

void Game::processInput() 
{
    m_window.pollEvents();
    
    if(m_window.consumeKeyPress(Key::Esc))
    {
        m_window.toggleCursor();
    }

    if(!m_window.isCursorEnabled())
    {
        if(m_window.isKeyPressed(Key::W))
        {
            m_camera.move(m_camera.getFront(), m_dt);
        }
        if(m_window.isKeyPressed(Key::A))
        {
            m_camera.move(-m_camera.getRight(), m_dt);
        }
        if(m_window.isKeyPressed(Key::S))
        {
            m_camera.move(-m_camera.getFront(), m_dt);
        }
        if(m_window.isKeyPressed(Key::D))
        {
            m_camera.move(m_camera.getRight(), m_dt);
        }
        if(m_window.isKeyPressed(Key::Space))
        {
            m_camera.move(m_camera.getUp(), m_dt);
        }
        if(m_window.isKeyPressed(Key::LCtrl))
        {
            m_camera.move(-m_camera.getUp(), m_dt);
        }

        float dx = (float) m_window.consumeDx();
        float dy = (float) m_window.consumeDy();
        m_camera.rotate(dx, dy);
    }
    else
    {
        m_window.consumeDx();
        m_window.consumeDy();
    }
}

void Game::update()
{

}

void Game::render()
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_testShader->use();
    m_testShader->setMat4("view", m_camera.getViewMatrix());
    m_testShader->setMat4("projection", m_camera.getProjectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureAtlas::instance().getID());
    m_testShader->setInt("atlas", 0);

    ChunkCoord coord = m_testChunk->getCoords();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(coord.x, coord.y, coord.z) * float(Chunk::SIZE));
    m_testShader->setMat4("model", model);
    m_chunkMesh->draw();

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
