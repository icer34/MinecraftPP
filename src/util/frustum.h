/**
 * @file frustum.h
 * @brief Camera view frustum, used to cull chunks that are off-screen.
 */

#pragma once

#include "glm/glm.hpp"
#include <array>
#include <cmath>
#include <glm/gtc/matrix_access.hpp>
#include <iostream>
#include <vector>

#include "graphics/camera.h"

/**
 * @brief Plane in Hessian normal form: the points p with `dot(normal, p) + dist == 0`.
 *
 * Points with a positive signed distance are on the side the normal points to.
 */
struct Plane
{
    glm::vec3 normal; ///< Unit normal of the plane.
    float dist;       ///< Signed distance term: `dot(normal, p) + dist` is the distance of p.

    /**
     * @brief Builds a normalized Plane from the raw (A, B, C, D) coefficients of
     * Ax + By + Cz + D = 0.
     */
    static Plane fromCoefficients(glm::vec4 coeffs)
    {
        float length = glm::length(glm::vec3(coeffs));
        return Plane{glm::vec3(coeffs) / length, coeffs.w / length};
    }
};

/**
 * @brief Names of the six frustum planes.
 */
enum class FrustumPlane : uint8_t
{
    LEFT = 0,
    RIGHT,
    TOP,
    BOTTOM,
    NEAR,
    FAR
};

/**
 * @brief The six planes of a camera's view frustum, with normals pointing inwards.
 *
 * Built from the camera's view-projection matrix (Gribb-Hartmann method). Only valid for
 * the camera state at construction time: build a new one every frame.
 */
class Frustum
{
public:
    /**
     * @brief Extracts the frustum planes from the camera's current view and projection.
     */
    Frustum(const Camera &cam)
    {
        glm::mat4 proj = cam.getProjectionMatrix();
        glm::mat4 view = cam.getViewMatrix();
        glm::mat4 m = proj * view;

        glm::vec4 row0 = glm::row(m, 0);
        glm::vec4 row1 = glm::row(m, 1);
        glm::vec4 row2 = glm::row(m, 2);
        glm::vec4 row3 = glm::row(m, 3); // the "w" row

        _planes[0] = Plane::fromCoefficients(row3 + row0); // left
        _planes[1] = Plane::fromCoefficients(row3 - row0); // right
        _planes[2] = Plane::fromCoefficients(row3 - row1); // top
        _planes[3] = Plane::fromCoefficients(row3 + row1); // bottom
        // reverse-Z with a [0, 1] depth range (see Camera::getProjectionMatrix()): visible
        // points have 0 <= z <= w, with z = w on the near plane and z = 0 on the far plane
        _planes[4] = Plane::fromCoefficients(row3 - row2); // near
        _planes[5] = Plane::fromCoefficients(row2);        // far
    }

    /**
     * @brief Tests whether a chunk's bounding box is at least partly inside the frustum.
     *
     * Conservative: it can return true for a box that is actually just outside the frustum
     * near a corner, but never returns false for a visible box.
     *
     * @param coord coordinates of the chunk; its box spans the full world height
     */
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

    /**
     * @brief Returns one plane of the frustum.
     */
    Plane getPlane(FrustumPlane planeDir) { return _planes[static_cast<size_t>(planeDir)]; }

    /**
     * @brief All six planes, in the order left, right, top, bottom, near, far.
     */
    std::array<Plane, 6> planes() const { return _planes; }

private:
    std::array<Plane, 6> _planes;
};