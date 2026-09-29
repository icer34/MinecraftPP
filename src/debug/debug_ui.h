#pragma once

#include <glm/glm.hpp>

struct DebugFrameInfo
{
    //* timings, in seconds
    /// time between the starts of the last two frames: the dt given to the simulation. Includes
    /// the vsync wait, so it stays at ~16.7 ms at 60 Hz whatever the real cost of a frame
    float frameTime;
    /// CPU time of the previous frame (input, update, render), without the vsync wait. Can
    /// still include some driver waits: drivers often block inside a GL call rather than in the
    /// buffer swap
    float cpuTime;
    /// GPU time of a frame a few frames old (see GpuTimer), not affected by the vsync
    float gpuTime;

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
     * @brief Stores the info of the frame and feeds the timing averages, has to be called every
     * frame, even when the panel is hidden
     *
     * @param info debug info of the current frame
     */
    void recordFrame(const DebugFrameInfo &info);

    /** @brief Draws the panel from the last recorded frame. */
    void render();

private:
    DebugFrameInfo _info{};

    //* timing averages, over a window of about 1 second
    float _fps = 0.0f;
    float _msPerFrame = 0.0f;
    float _cpuMs = 0.0f;
    float _gpuMs = 0.0f;

    int _frameCount = 0;
    float _fpsTimer = 0.0f;
    float _cpuTimeSum = 0.0f;
    float _gpuTimeSum = 0.0f;
};
