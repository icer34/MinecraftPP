#pragma once

class AABB
{
public:
    AABB(glm::vec3 halfExtents) { m_halfExtents = halfExtents; }

    // pos is the entity's feet (bottom-center), not the box's geometric center
    glm::vec3 getMin(glm::vec3 pos) const
    {
        return glm::vec3(pos.x - m_halfExtents.x, pos.y, pos.z - m_halfExtents.z);
    }

    glm::vec3 getMax(glm::vec3 pos) const
    {
        return glm::vec3(pos.x + m_halfExtents.x,
                         pos.y + 2.0f * m_halfExtents.y,
                         pos.z + m_halfExtents.z);
    }

    glm::vec3 getHalfExtents() const { return m_halfExtents; }

    bool intersects(const AABB &other, glm::vec3 posA, glm::vec3 posB) const
    {
        glm::vec3 minA = getMin(posA), maxA = getMax(posA);
        glm::vec3 minB = other.getMin(posB), maxB = other.getMax(posB);

        return minA.x < maxB.x && maxA.x > minB.x && minA.y < maxB.y && maxA.y > minB.y
            && minA.z < maxB.z && maxA.z > minB.z;
    }

private:
    glm::vec3 m_halfExtents;
};