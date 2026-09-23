/**
 * @file block_outline.h
 * @brief Wireframe cube drawn around the block the player is looking at.
 */

#pragma once

#include <glm/glm.hpp>

#include "graphics/camera.h"
#include "graphics/shader.h"

/**
 * @brief Draws the 12 edges of a unit cube around a block, as `GL_LINES`.
 *
 * Owns its own VAO/VBO and shader. Must be constructed while a GL context is current.
 */
class BlockOutline
{
public:
    /**
     * @brief Uploads the cube edge vertices to the GPU and loads the outline shader.
     */
    BlockOutline();

    /**
     * @brief Draws the outline around one block.
     *
     * @param pos world position of the block's minimum corner (integer block coordinates)
     * @param cam camera providing the view and projection matrices
     */
    void draw(glm::vec3 pos, const Camera &cam);

private:
    unsigned int _vao;
    unsigned int _vbo;
    Shader _shader;
};