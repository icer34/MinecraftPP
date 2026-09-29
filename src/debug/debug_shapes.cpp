#include "debug_shapes.h"

#include <cmath>

#include "debug_draw.h"
#include "game/chunk.h"
#include "util/frustum.h"

void DebugShapes::chunkBorders(DebugDraw &draw, glm::vec3 pos)
{
    const glm::vec3 GRID_COLOR(1.0f, 1.0f, 0.0f);     // yellow: grid on the current chunk's walls
    const glm::vec3 CORNER_COLOR(0.0f, 0.5f, 1.0f);   // blue: the current chunk's 4 corners
    const glm::vec3 NEIGHBOR_COLOR(1.0f, 0.0f, 0.0f); // red: corners of the neighbouring chunks
    const float S = Chunk::SIZE;
    const float H = Chunk::HEIGHT;
    const float GRID_STEP = 2.0f; // spacing of the wall grid, both horizontally and vertically

    // floor, not a cast to int: x = -0.5 must give chunk -1, not 0
    glm::vec3 min(std::floor(pos.x / S) * S, 0.0f, std::floor(pos.z / S) * S);
    glm::vec3 max = min + glm::vec3(S, H, S);

    // current chunk: horizontal rings on its 4 walls
    for (float y = 0.0f; y <= H; y += GRID_STEP)
    {
        auto color = (int)y % 16 == 0 ? CORNER_COLOR : GRID_COLOR;

        draw.line({min.x, y, min.z}, {max.x, y, min.z}, color);
        draw.line({max.x, y, min.z}, {max.x, y, max.z}, color);
        draw.line({max.x, y, max.z}, {min.x, y, max.z}, color);
        draw.line({min.x, y, max.z}, {min.x, y, min.z}, color);
    }

    // current chunk: vertical lines on its 4 walls. Stops before S: the corners are drawn by
    // the loop below, in their own color
    for (float t = GRID_STEP; t < S; t += GRID_STEP)
    {
        draw.line({min.x + t, 0.0f, min.z}, {min.x + t, H, min.z}, GRID_COLOR); // wall z = min
        draw.line({min.x + t, 0.0f, max.z}, {min.x + t, H, max.z}, GRID_COLOR); // wall z = max
        draw.line({min.x, 0.0f, min.z + t}, {min.x, H, min.z + t}, GRID_COLOR); // wall x = min
        draw.line({max.x, 0.0f, min.z + t}, {max.x, H, min.z + t}, GRID_COLOR); // wall x = max
    }

    // vertical lines at the corners of the 3x3 chunks around: i, j = corner index along x, z
    for (int i = -1; i <= 2; i++)
    {
        for (int j = -1; j <= 2; j++)
        {
            bool currentCorner = (i == 0 || i == 1) && (j == 0 || j == 1);
            float x = min.x + i * S;
            float z = min.z + j * S;
            draw.line({x, 0.0f, z}, {x, H, z}, currentCorner ? CORNER_COLOR : NEIGHBOR_COLOR);
        }
    }
}

void DebugShapes::frustum(DebugDraw &draw, const Frustum &frustum, glm::vec3 color)
{
    const std::array<glm::vec3, 8> &c = frustum.corners();

    // two corners share an edge when their indices differ by exactly one bit (see
    // Frustum::corners()): 8 corners * 3 bits / 2 = 12 edges. Each edge is drawn once, from its
    // corner where that bit is 0
    for (int i = 0; i < 8; i++)
    {
        for (int bit = 1; bit < 8; bit <<= 1)
        {
            if (!(i & bit))
                draw.line(c[i], c[i | bit], color, true);
        }
    }
}