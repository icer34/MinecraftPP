/**
 * @file aabb.h
 * @brief Axis-aligned bounding box used for entity hitboxes.
 */

#pragma once

/**
 * @brief Axis-aligned bounding box anchored at an entity's feet.
 *
 * The box only stores its size: its position is passed to each method. That position is the
 * bottom-center of the box (the entity's feet), not its geometric center. The box spans
 * `pos.y` to `pos.y + 2 * halfExtents.y` vertically.
 */
class AABB
{
public:
    /**
     * @param halfExtents half of the box size on each axis
     */
    AABB(glm::vec3 halfExtents) { _halfExtents = halfExtents; }

    /**
     * @brief Minimum corner of the box.
     *
     * @param pos the entity's feet (bottom-center), not the box's geometric center
     */
    glm::vec3 getMin(glm::vec3 pos) const
    {
        return glm::vec3(pos.x - _halfExtents.x, pos.y, pos.z - _halfExtents.z);
    }

    /**
     * @brief Maximum corner of the box.
     *
     * @param pos the entity's feet (bottom-center), not the box's geometric center
     */
    glm::vec3 getMax(glm::vec3 pos) const
    {
        return glm::vec3(
            pos.x + _halfExtents.x, pos.y + 2.0f * _halfExtents.y, pos.z + _halfExtents.z);
    }

    /** @brief Half of the box size on each axis. */
    glm::vec3 getHalfExtents() const { return _halfExtents; }

    /**
     * @brief Tests whether this box overlaps another one. Touching boxes do not overlap.
     *
     * @param other the other box
     * @param posA feet position of this box
     * @param posB feet position of the other box
     */
    bool intersects(const AABB &other, glm::vec3 posA, glm::vec3 posB) const
    {
        glm::vec3 minA = getMin(posA), maxA = getMax(posA);
        glm::vec3 minB = other.getMin(posB), maxB = other.getMax(posB);

        return minA.x < maxB.x && maxA.x > minB.x && minA.y < maxB.y && maxA.y > minB.y
            && minA.z < maxB.z && maxA.z > minB.z;
    }

private:
    glm::vec3 _halfExtents;
};