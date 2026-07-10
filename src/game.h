#pragma once

#include "util/window.h"
#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "graphics/camera.h"
#include "game/chunk.h"
#include "graphics/chunk_mesh.h"
#include "graphics/chunk_mesher.h"

#include <memory>

class Game 
{
public:
    Game();

    void run();

private:
    Window m_window;
    Camera m_camera;

    std::unique_ptr<Shader> m_testShader;
    std::unique_ptr<Chunk> m_testChunk;
    std::unique_ptr<ChunkMesh> m_chunkMesh;
    
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