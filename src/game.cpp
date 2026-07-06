#include "game.h"

#include <glad/glad.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "game/blocks.h"
#include "game/chunk.h"
#include "util/key_codes.h"

Game::Game() 
    : m_window(1600, 900, "MinecraftPP", false)
    , m_camera(glm::vec3(0.0f, 0.0f, 3.0f), m_window.getAspectRatio())
{
    Blocks::registerAll();

    m_testShader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");
    m_testMesh = std::make_unique<Mesh>();

    std::vector<float> cubeVertices = {
        -0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,
         0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,
         0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
        -0.5f, 0.5f,-0.5f,  0,0,-1,  0,1,
        -0.5f,-0.5f, 0.5f,  0,0,1,   0,0,
         0.5f,-0.5f, 0.5f,  0,0,1,   1,0,
         0.5f, 0.5f, 0.5f,  0,0,1,   1,1,
        -0.5f, 0.5f, 0.5f,  0,0,1,   0,1,
    };

    std::vector<unsigned int> cubeIndices = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,4,7, 7,3,0,
        1,5,6, 6,2,1,
        3,2,6, 6,7,3,
        0,1,5, 5,4,0
    };

    m_testShader->use();
    m_testShader->setMat4("model", m_model);

    m_testMesh->update(MeshData{cubeVertices, cubeIndices});
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
    
    //keyboard inputs
    if(m_window.consumeKeyPress(Key::Esc))
    {
        m_window.toggleCursor();
    }
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

    //mouse inputs
    float dx = (float) m_window.consumeDx();
    float dy = (float) m_window.consumeDy();
    m_camera.rotate(dx, dy);
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

    m_testMesh->draw();

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
