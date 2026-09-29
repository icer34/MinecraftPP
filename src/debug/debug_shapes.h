#pragma once

#include <glm/glm.hpp>

class DebugDraw;
class Frustum;

/// @brief contains all the methods that can add the debug shapes vertices to the draw itself
namespace DebugShapes
{
/**
 * @brief Adds chunks borders around pos (yellow), TODO:
 *
 * @param draw The data in which the shape will be added
 * @param pos Position of the player
 */
void chunkBorders(DebugDraw &draw, glm::vec3 pos);

/**
 * @brief Adds the 12 edges of a frustum, always on top: they cross the terrain.
 *
 * @param draw The data in which the shape will be added
 * @param frustum The frustum to draw, e.g. the one frozen by DebugSettings::freezeCulling
 * @param color Color of the edges
 */
void frustum(DebugDraw &draw, const Frustum &frustum, glm::vec3 color);
} // namespace DebugShapes