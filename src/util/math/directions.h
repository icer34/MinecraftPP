/**
 * @file directions.h
 * @brief Block face directions, axes, and their unit vectors.
 */

#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <iostream>

/**
 * @brief The six directions of a block face.
 *
 * The underlying values are used as indices (e.g. into DIRECTION_VECTORS or
 * BlockType::textures), so their order must not change.
 */
enum class Direction : uint8_t
{
    NORTH = 0, ///< -z
    SOUTH,     ///< +z
    EAST,      ///< +x
    WEST,      ///< -x
    TOP,       ///< +y
    BOTTOM     ///< -y
};

/**
 * @brief The three world axes.
 */
enum class Axis : uint8_t
{
    X = 0,
    Y,
    Z
};

/**
 * @brief Unit vector of each Direction, indexed by its underlying value.
 */
constexpr std::array<glm::ivec3, 6> DIRECTION_VECTORS = {{
    {0, 0, -1}, // north
    {0, 0, 1},  // south
    {1, 0, 0},  // east
    {-1, 0, 0}, // west
    {0, 1, 0},  // top
    {0, -1, 0}  // bottom
}};

/**
 * @brief Returns the unit vector pointing in `dir`.
 */
inline glm::ivec3 getDirectionVector(Direction dir)
{
    size_t idx = static_cast<size_t>(dir);
    return DIRECTION_VECTORS[idx];
}

/**
 * @brief All six directions, in enum order.
 */
constexpr std::array<Direction, 6> ALL_DIRECTIONS = {Direction::NORTH,
                                                     Direction::SOUTH,
                                                     Direction::EAST,
                                                     Direction::WEST,
                                                     Direction::TOP,
                                                     Direction::BOTTOM};

/**
 * @brief The four horizontal directions, i.e. the neighboring chunks of a chunk.
 *
 * This order is also the order of the `neighbors` arrays used by World and ChunkMesher.
 */
constexpr std::array<Direction, 4> CARDINAL_DIRECTIONS
    = {Direction::NORTH, Direction::SOUTH, Direction::EAST, Direction::WEST};