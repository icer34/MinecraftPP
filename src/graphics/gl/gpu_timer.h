/**
 * @file gpu_timer.h
 * @brief Measures the GPU time of a sequence of GL commands, without stalling the CPU.
 */

#pragma once

#include <array>

#include "gl_objects.h"

/**
 * @brief Measures the GPU execution time of the GL commands issued between begin() and end(),
 * once per frame.
 *
 * The GPU timestamps the commands itself (`GL_TIMESTAMP` queries), so the measure excludes
 * the vsync wait and any time the GPU spends idle before begin().
 *
 * The GPU runs a few frames behind the CPU: reading the result of the current frame would
 * block until the GPU catches up. The queries are therefore rotated over FRAMES_IN_FLIGHT
 * slots, and a slot is read back only when it is reused, FRAMES_IN_FLIGHT frames later, when
 * its result is ready.
 *
 * Must be constructed after the GL context exists (i.e. after the Window).
 */
class GpuTimer
{
public:
    GpuTimer();

    /** @brief Starts the measure of the current frame. Call once per frame, before end(). */
    void begin();
    /** @brief Ends the measure of the current frame. */
    void end();

    /**
     * @brief GPU time of the most recent measured frame, in seconds (0 until the first
     * result is available).
     *
     * That frame is FRAMES_IN_FLIGHT frames old.
     */
    float getLastTime() const { return _lastTime; }

private:
    static constexpr int FRAMES_IN_FLIGHT = 3;

    std::array<GLQuery, FRAMES_IN_FLIGHT> _startQueries;
    std::array<GLQuery, FRAMES_IN_FLIGHT> _endQueries;
    // true if the slot holds a measure that was not read back yet
    std::array<bool, FRAMES_IN_FLIGHT> _pending{};

    int _current = 0;
    float _lastTime = 0.0f;
};
