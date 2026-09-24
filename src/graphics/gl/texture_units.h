/**
 * @file texture_units.h
 * @brief Texture unit of every sampler, shared by all the shaders.
 */

#pragma once

#include <glad/glad.h>

/**
 * @brief Texture unit of each sampler, bound with `glBindTextureUnit(unit, texture)`.
 *
 * Every texture gets its own unit, so two shaders can never use the same unit for two
 * different textures by mistake. The shaders read these units from
 * `shaders/common/texture_units.glsl` (`layout(binding = ...)`), which must be kept in sync.
 */
enum TextureUnit : GLuint
{
    BLOCK_ATLAS = 0,
    BLOCK_COLORMAP = 1,
    SHADOW_MAP = 2,
    SOLID_COLOR = 3, ///< opaque scene color, sampled by the water
    SOLID_DEPTH = 4, ///< opaque scene depth, sampled by the water
    HUD_ATLAS = 5,   ///< icon or font atlas of the current HUD batch
};
