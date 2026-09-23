/**
 * @file cascaded_shadow_map.h
 * @brief Cascaded shadow maps for the directional sun light.
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iostream>
#include <vector>

#include "camera.h"

/**
 * @brief Shadow map split into several cascades along the camera's view distance.
 *
 * The view frustum is split into depth() slices, with a mix of logarithmic and uniform
 * splits. Each slice gets its own light-space projection, so nearby shadows get more
 * resolution than distant ones. All cascades are stored as layers of a single depth texture
 * array, rendered in one pass with a geometry shader.
 */
class CascadedShadowMap
{
public:
    /**
     * @brief Creates the depth texture array and its framebuffer.
     */
    CascadedShadowMap();
    ~CascadedShadowMap();

    /**
     * @brief Recomputes the cascade splits and light matrices for the current camera.
     *
     * Call once per frame, before the shadow pass.
     *
     * @param cam the player camera
     * @param lightDir direction the light travels in (from the sun towards the scene)
     */
    void update(const Camera &cam, const glm::vec3 &lightDir);

    /**
     * @brief Light view-projection matrix of each cascade, from nearest to farthest.
     */
    const std::vector<glm::mat4> &getLightVPMatrices() const { return _lightVPMatrices; }

    /**
     * @brief View-space far distance of each cascade, used by the shader to pick a cascade.
     */
    const std::vector<float> &getCutoffDists() const { return _cutoffDist; }

    /** @brief Width and height of each cascade, in texels. */
    unsigned int size() const { return _textureSize; }
    /** @brief Number of cascades (layers of the texture array). */
    unsigned int depth() const { return _depth; }
    /** @brief OpenGL name of the framebuffer to render the shadow pass into. */
    unsigned int getFrameBufferID() const { return _fboID; }
    /** @brief OpenGL name of the depth texture array (`GL_TEXTURE_2D_ARRAY`). */
    unsigned int getTextureID() const { return _texID; }

private:
    glm::mat4 getLightVPMatrix(const Camera &cam,
                               float zNear,
                               float zFar,
                               const glm::vec3 &lightDir);

    unsigned int _depth = 5;
    unsigned int _textureSize = 4096;

    std::vector<glm::mat4> _lightVPMatrices;
    std::vector<float> _cutoffDist;

    unsigned int _texID;
    unsigned int _fboID;
};