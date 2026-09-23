/**
 * @file drawable.h
 * @brief Minimal interface for anything that can issue its own draw call.
 */

#pragma once

/**
 * @brief Interface for GPU-backed objects that know how to draw themselves.
 *
 * The caller is responsible for binding the shader and setting its uniforms before calling
 * draw().
 */
class Drawable
{
public:
    virtual ~Drawable() = default;

    /**
     * @brief Issues the draw call(s) for this object with the currently bound shader.
     */
    virtual void draw() = 0;
};