#pragma once

#include "glm/glm.hpp"
#include <array>
#include <cmath>
#include <glm/gtc/matrix_access.hpp>
#include <iostream>
#include <vector>

#include "graphics/camera.h"

struct Plane
{
    glm::vec3 normal;
    float dist; // distance between the origin and the nearest point on the plane

    // builds a normalized Plane from the raw (A,B,C,D) coefficients of Ax+By+Cz+D=0
    static Plane fromCoefficients(glm::vec4 coeffs)
    {
        float length = glm::length(glm::vec3(coeffs));
        return Plane{glm::vec3(coeffs) / length, coeffs.w / length};
    }
};

enum class FrustumPlane : uint8_t
{
    TOP = 0,
    BOTTOM,
    NEAR,
    FAR,
    LEFT,
    RIGHT
};

class Frustum
{
public:
    Frustum(const Camera &cam)
    {
        glm::mat4 proj = cam.getProjectionMatrix();
        glm::mat4 view = cam.getViewMatrix();
        glm::mat4 m = proj * view;

        // temporary: catch the exact frame a degenerate/NaN VP matrix shows up
        bool hasNan = false;
        for (int col = 0; col < 4 && !hasNan; col++)
            for (int row = 0; row < 4; row++)
                if (std::isnan(m[col][row]) || std::isinf(m[col][row]))
                {
                    hasNan = true;
                    break;
                }
        if (hasNan)
        {
            glm::vec3 pos = cam.getPos();
            glm::vec3 front = cam.getFront();
            std::cout << "[frustum-debug] NaN/Inf in VP matrix! pos=(" << pos.x << "," << pos.y
                      << "," << pos.z << ") front=(" << front.x << "," << front.y << "," << front.z
                      << ") yaw=" << cam.getYaw() << " pitch=" << cam.getPitch()
                      << " aspect=" << cam.getAspectRatio() << " fov=" << cam.getFOV() << std::endl;
        }

        glm::vec4 row0 = glm::row(m, 0);
        glm::vec4 row1 = glm::row(m, 1);
        glm::vec4 row2 = glm::row(m, 2);
        glm::vec4 row3 = glm::row(m, 3); // the "w" row

        m_left = Plane::fromCoefficients(row3 + row0);
        m_right = Plane::fromCoefficients(row3 - row0);
        m_bottom = Plane::fromCoefficients(row3 + row1);
        m_top = Plane::fromCoefficients(row3 - row1);
        m_near = Plane::fromCoefficients(row3 + row2);
        m_far = Plane::fromCoefficients(row3 - row2);
    }

    bool isChunkInside(const ChunkCoord coord)
    {
        glm::vec3 minBox = glm::vec3(coord.x * Chunk::SIZE, 0.0f, coord.z * Chunk::SIZE);
        glm::vec3 maxBox
            = glm::vec3((coord.x + 1) * Chunk::SIZE, Chunk::HEIGHT, (coord.z + 1) * Chunk::SIZE);

        for (const Plane &plane : planes())
        {
            // the AABB corner furthest along the plane's normal -- if even that corner is
            // outside (behind) the plane, the whole box is outside it, so it can't be visible
            glm::vec3 positiveVertex(plane.normal.x >= 0.0f ? maxBox.x : minBox.x,
                                     plane.normal.y >= 0.0f ? maxBox.y : minBox.y,
                                     plane.normal.z >= 0.0f ? maxBox.z : minBox.z);

            if (glm::dot(plane.normal, positiveVertex) + plane.dist < 0.0f)
                return false;
        }

        return true;
    }

    // temporary debug helper -- prints each plane's normal/dist and the AABB test result
    // for one chunk, to catch the exact frustum state when everything gets culled.
    void debugDumpChunk(const ChunkCoord coord) const
    {
        glm::vec3 minBox = glm::vec3(coord.x * Chunk::SIZE, 0.0f, coord.z * Chunk::SIZE);
        glm::vec3 maxBox
            = glm::vec3((coord.x + 1) * Chunk::SIZE, Chunk::HEIGHT, (coord.z + 1) * Chunk::SIZE);

        static constexpr const char *names[6] = {"near", "far", "top", "bottom", "left", "right"};

        int i = 0;
        for (const Plane &plane : planes())
        {
            glm::vec3 positiveVertex(plane.normal.x >= 0.0f ? maxBox.x : minBox.x,
                                     plane.normal.y >= 0.0f ? maxBox.y : minBox.y,
                                     plane.normal.z >= 0.0f ? maxBox.z : minBox.z);
            float result = glm::dot(plane.normal, positiveVertex) + plane.dist;

            std::cout << "  [" << names[i] << "] normal=(" << plane.normal.x << ","
                      << plane.normal.y << "," << plane.normal.z << ") dist=" << plane.dist
                      << " -> test=" << result << (result < 0.0f ? " FAIL" : " pass") << std::endl;
            i++;
        }
    }

    Plane getPlane(FrustumPlane planeDir)
    {
        switch (planeDir)
        {
        case FrustumPlane::TOP:
            return m_top;
        case FrustumPlane::BOTTOM:
            return m_bottom;
        case FrustumPlane::NEAR:
            return m_near;
        case FrustumPlane::FAR:
            return m_far;
        case FrustumPlane::LEFT:
            return m_left;
        case FrustumPlane::RIGHT:
            return m_right;
        }
    }

    std::vector<Plane> planes() const
    {
        return std::vector<Plane>{m_near, m_far, m_top, m_bottom, m_left, m_right};
    }

private:
    Plane m_near;
    Plane m_far;
    Plane m_top;
    Plane m_bottom;
    Plane m_left;
    Plane m_right;
};