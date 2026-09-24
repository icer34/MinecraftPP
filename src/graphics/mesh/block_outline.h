/**
 * @file block_outline.h
 * @brief Outline drawn around the block the player is looking at.
 */

#pragma once

#include <glm/glm.hpp>

#include "graphics/gl/gl_objects.h"
#include "graphics/shader.h"

/**
 * @brief Draws the 12 edges of a unit cube around a block, with a constant width in pixels.
 *
 * Each edge is a thin quad facing the camera, rather than a `GL_LINES` line: line widths above
 * 1 are deprecated and not supported everywhere. The quads are generated in the vertex shader
 * from `gl_VertexID`, so no vertex buffer is needed.
 *
 * Reads the camera from the FrameData uniform buffer. Must be constructed while a GL context is
 * current.
 */
class BlockOutline
{
public:
    /**
     * @brief Loads the outline shader.
     */
    BlockOutline();

    /**
     * @brief Draws the outline around one block, into the currently bound framebuffer.
     *
     * The FrameData of the current frame must already be uploaded.
     *
     * @param pos world position of the block's minimum corner (integer block coordinates)
     */
    void draw(glm::vec3 pos);

private:
    GLVertexArray _vao; // empty, but a VAO must be bound to draw in a core profile
    Shader _shader;
};
