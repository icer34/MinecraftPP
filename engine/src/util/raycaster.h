#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "core/block_registry.h"
#include "core/voxel_world.h"

struct RayCastResult
{
    bool hit;
    glm::vec3 targetPos;
    glm::vec3 targetNorm;
    uint16_t targetBlockID;
};

class RayCaster
{
public:
    RayCaster(const IVoxelWorld &world)
        : m_world(world)
    {
    }

    RayCastResult cast(glm::vec3 origin, glm::vec3 dir, float reach)
    {
        if (glm::length(dir) == 0.0f)
            return RayCastResult{false, glm::vec3(0.0f), glm::vec3(0.0f), 0};

        dir = glm::normalize(dir);

        // starting voxel
        int x = (int)floor(origin.x);
        int y = (int)floor(origin.y);
        int z = (int)floor(origin.z);

        int stepX = dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0);
        int stepY = dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0);
        int stepZ = dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0);

        // distance between 2 face intersections on an axis
        float tDeltaX = (stepX == 0) ? std::numeric_limits<float>::infinity() : abs(1.0f / dir.x);
        float tDeltaY = (stepY == 0) ? std::numeric_limits<float>::infinity() : abs(1.0f / dir.y);
        float tDeltaZ = (stepZ == 0) ? std::numeric_limits<float>::infinity() : abs(1.0f / dir.z);

        // tMax = distance until the next face on each axis
        float nextVoxelBoundaryX = (stepX > 0) ? (x + 1.0f) : (float)x;
        float nextVoxelBoundaryY = (stepY > 0) ? (y + 1.0f) : (float)y;
        float nextVoxelBoundaryZ = (stepZ > 0) ? (z + 1.0f) : (float)z;

        float tMaxX = (stepX == 0) ? std::numeric_limits<float>::infinity()
                                   : (nextVoxelBoundaryX - origin.x) / dir.x;
        float tMaxY = (stepY == 0) ? std::numeric_limits<float>::infinity()
                                   : (nextVoxelBoundaryY - origin.y) / dir.y;
        float tMaxZ = (stepZ == 0) ? std::numeric_limits<float>::infinity()
                                   : (nextVoxelBoundaryZ - origin.z) / dir.z;

        auto hitNormal = glm::vec3(0.0f);
        float t = 0.0f;
        while (t <= reach)
        {

            uint16_t blockID = m_world.getBlock(glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f));
            if (BlockRegistry::instance().get(blockID).isSolid)
            {
                return RayCastResult{true, glm::vec3(x, y, z), hitNormal, blockID};
            }

            if (tMaxX < tMaxY && tMaxX < tMaxZ)
            {
                x += stepX;
                t = tMaxX;
                tMaxX += tDeltaX;
                hitNormal = glm::vec3(-stepX, 0, 0);
            }
            else if (tMaxY < tMaxZ)
            {
                y += stepY;
                t = tMaxY;
                tMaxY += tDeltaY;
                hitNormal = glm::vec3(0, -stepY, 0);
            }
            else
            {
                z += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                hitNormal = glm::vec3(0, 0, -stepZ);
            }
        }

        return RayCastResult{false, glm::vec3(0.0f), glm::vec3(0.0f), 0};
    }

private:
    const IVoxelWorld &m_world;
};
