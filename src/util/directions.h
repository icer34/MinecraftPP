#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>

enum class Direction : uint8_t
{
    NORTH = 0, //  -z
    SOUTH,     //  +z
    EAST,      //  +x
    WEST,      //  -x
    TOP,       //  +y
    BOTTOM     //  -y
};

constexpr std::array<glm::ivec3, 6> DIRECTION_VECTORS = {{
    {0, 0, -1}, // north
    {0, 0, 1},  // south
    {1, 0, 0},  // east
    {-1, 0, 0}, // west
    {0, 1, 0},  // top
    {0, -1, 0}  // bottom
}};

inline glm::ivec3 getDirectionVector(Direction dir)
{
    return DIRECTION_VECTORS[static_cast<size_t>(dir)];
}

constexpr std::array<Direction, 6> ALL_DIRECTIONS = {Direction::NORTH,
                                                     Direction::SOUTH,
                                                     Direction::EAST,
                                                     Direction::WEST,
                                                     Direction::TOP,
                                                     Direction::BOTTOM};