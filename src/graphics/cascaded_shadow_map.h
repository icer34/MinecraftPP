/**
 * @file cascaded_shadow_map.h
 * @brief Cascaded shadow maps for the directional sun light.
 */

#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <array>
#include <span>

#include "camera.h"
#include "gl/gl_objects.h"

/**
 * @brief Shadow map split into several cascades along the camera's view distance.
 *
 * The view frustum is split into CASCADE_COUNT slices, with a mix of logarithmic and uniform
 * splits. Each slice gets its own light-space projection, so nearby shadows get more
 * resolution than distant ones. All cascades are stored as layers of a single depth texture
 * array, rendered in one pass with an instanced geometry shader (one invocation per cascade).
 *
 * The depth is a regular one, in [0, 1] with 0 closest to the light: clear it to 1 and test
 * with `GL_LESS`, unlike the reverse-Z scene.
 */
class CascadedShadowMap
{
public:
    static constexpr unsigned int CASCADE_COUNT = 5;
    static constexpr unsigned int TEXTURE_SIZE = 4096;

    /**
     * @brief Creates the depth texture array and its framebuffer.
     */
    CascadedShadowMap();

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
    std::span<const glm::mat4, CASCADE_COUNT> getLightVPMatrices() const
    {
        return _lightVPMatrices;
    }

    /**
     * @brief View-space far distance of each cascade, used by the shader to pick a cascade.
     */
    std::span<const float, CASCADE_COUNT> getCutoffDists() const { return _cutoffDist; }

    /** @brief OpenGL name of the framebuffer to render the shadow pass into. */
    unsigned int getFrameBufferID() const { return _fbo.id(); }
    /** @brief OpenGL name of the depth texture array (`GL_TEXTURE_2D_ARRAY`). */
    unsigned int getTextureID() const { return _tex.id(); }

private:
    glm::mat4 getLightVPMatrix(const Camera &cam,
                               float zNear,
                               float zFar,
                               const glm::vec3 &lightDir);

    std::array<glm::mat4, CASCADE_COUNT> _lightVPMatrices{};
    std::array<float, CASCADE_COUNT> _cutoffDist{};

    GLTexture _tex;
    GLFramebuffer _fbo;
};