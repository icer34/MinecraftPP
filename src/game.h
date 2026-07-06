#pragma once

#include "util/window.h"
#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "graphics/camera.h"

class Game 
{
public:
    Game();

    void run();

private:
    Window m_window;
    Camera m_camera;

    glm::mat4 m_model = glm::mat4(1.0f);
    
    std::unique_ptr<Shader> m_testShader;
    std::unique_ptr<Mesh> m_testMesh;
    
    void processInput();
    void update();
    void render();
    
    float m_dt;
    float m_lastFrameTime;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    float m_displayedFps = 0.0f;

    void updateFpsCounter();
};