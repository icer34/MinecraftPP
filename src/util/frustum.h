#pragma once

#include "glm/glm.hpp"
#include <array>
#include <glm/gtc/matrix_access.hpp>
#include <vector>

#include "camera.h"

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
        glm::mat4 m = cam.getProjectionMatrix() * cam.getViewMatrix();

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