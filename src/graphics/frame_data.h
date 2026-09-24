/**
 * @file frame_data.h
 * @brief Per-frame uniforms shared by every shader, stored in one uniform buffer.
 */

#pragma once

#include <glm/glm.hpp>

#include <cstddef>

#include "graphics/cascaded_shadow_map.h"
#include "graphics/gl/gl_objects.h"

/// Uniform buffer binding point of the FrameData block (`layout(binding = 0)` in GLSL).
inline constexpr GLuint FRAME_DATA_BINDING = 0;

/**
 * @brief C++ mirror of the `FrameData` uniform block of `shaders/common/frame_data.glsl`.
 *
 * The block uses the std140 layout: a `vec3` takes the space of a `vec4` unless a `float`
 * follows it, and every element of a `float` array takes 16 bytes. The members are ordered
 * so that no hidden padding is needed, and the cascade cutoff distances are packed 4 per
 * `vec4`. The static_asserts below catch any mismatch at compile time: the two definitions
 * must always be changed together.
 */
struct alignas(16) FrameData
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 invView;
    glm::mat4 invProjection;
    glm::mat4 lightSpaceMatrices[CascadedShadowMap::CASCADE_COUNT];
    glm::vec4 cutoffDist[2]; ///< cascade i is cutoffDist[i / 4][i % 4]
    glm::vec3 lightDir;
    float time; ///< seconds since startup
    glm::vec3 camPos;
    float zNear;
    glm::vec2 screenSize; ///< in pixels
    float zFar;
    float _pad0;
};

static_assert(CascadedShadowMap::CASCADE_COUNT == 5, "update CASCADE_COUNT in frame_data.glsl");
static_assert(offsetof(FrameData, view) == 0);
static_assert(offsetof(FrameData, projection) == 64);
static_assert(offsetof(FrameData, invView) == 128);
static_assert(offsetof(FrameData, invProjection) == 192);
static_assert(offsetof(FrameData, lightSpaceMatrices) == 256);
static_assert(offsetof(FrameData, cutoffDist) == 576);
static_assert(offsetof(FrameData, lightDir) == 608);
static_assert(offsetof(FrameData, time) == 620);
static_assert(offsetof(FrameData, camPos) == 624);
static_assert(offsetof(FrameData, zNear) == 636);
static_assert(offsetof(FrameData, screenSize) == 640);
static_assert(offsetof(FrameData, zFar) == 648);
static_assert(sizeof(FrameData) == 656);

/**
 * @brief The GPU uniform buffer holding the FrameData, bound to FRAME_DATA_BINDING.
 *
 * Every shader that includes `common/frame_data.glsl` reads it, without any per-shader call.
 */
class FrameDataBuffer
{
public:
    /**
     * @brief Creates the buffer and binds it to FRAME_DATA_BINDING.
     */
    FrameDataBuffer();

    /**
     * @brief Uploads the data of the current frame. Call once per frame, before drawing.
     */
    void upload(const FrameData &data);

private:
    GLBuffer _buffer;
};
