#pragma once

#include <glm/glm.hpp>

struct DebugFrameInfo
{
    float frameTime; // dt of the current frame in seconds

    glm::vec3 cameraPos;
    int loadedChunks;
    int renderedChunks;
    float pvNoise;
    float erosionNoise;
    float continentalnessNoise;

    //... will grow in the future
};

class Input;

class DebugUI
{
public:
    /**
     * @brief sets the DebugSettings flags according to the user's inputs
     *
     * @param window
     */
    void handleInput(Input &window);

    /**
     * @brief Feeds the FPS average, has to be called every frame
     *
     * @param dt duration of the last frame in seconds
     */
    void recordFrame(float dt);

    void render(const DebugFrameInfo &info);

private:
    float _fps = 0.0f;
    float _msPerFrame = 0.0f;
    int _frameCount = 0;
    float _fpsTimer = 0.0f;
};