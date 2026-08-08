#pragma once

#include <glm/glm.hpp>

#include "core/world.h"
#include "util/aabb.h"
#include "util/directions.h"

class Entity
{
public:
    Entity(glm::vec3 pos, glm::vec3 hitboxHalfExtents)
        : m_pos(pos),
          m_vel(0.0f),
          m_hitBox(hitboxHalfExtents),
          m_onGround(false)
    {
    }

    virtual ~Entity() = default;
    virtual void update(float dt, World &world) = 0;

    const AABB &getHitBox() const { return m_hitBox; }

protected:
    glm::vec3 m_pos;
    glm::vec3 m_vel;

    AABB m_hitBox;

    bool m_onGround;

    void moveAndCollide(glm::vec3 delta, World &world)
    {
        //*resolve collisions for each axis independantly to avoid being stuck in a corner
        m_pos.x += delta.x;
        resolveAxisCollision(Axis::X, delta.x, world);

        m_pos.z += delta.z;
        resolveAxisCollision(Axis::Z, delta.z, world);

        m_onGround = false;
        m_pos.y += delta.y;
        resolveAxisCollision(Axis::Y, delta.y, world);
    }

private:
    void resolveAxisCollision(Axis axis, float delta, World &world)
    {
        glm::vec3 min = m_hitBox.getMin(m_pos);
        glm::vec3 max = m_hitBox.getMax(m_pos);

        for (int x = (int)floor(min.x); x < (int)ceil(max.x); x++)
            for (int y = (int)floor(min.y); y < (int)ceil(max.y); y++)
                for (int z = (int)floor(min.z); z < (int)ceil(max.z); z++)
                {
                    if (!world.isBlockSolid(glm::vec3(x, y, z)))
                        continue;

                    if (axis == Axis::X)
                    {
                        m_pos.x = delta > 0.0f ? (float)x - m_hitBox.getHalfExtents().x
                                               : (float)(x + 1) + m_hitBox.getHalfExtents().x;
                        m_vel.x = 0.0f;
                    }
                    else if (axis == Axis::Z)
                    {
                        m_pos.z = delta > 0.0f ? (float)z - m_hitBox.getHalfExtents().z
                                               : (float)(z + 1) + m_hitBox.getHalfExtents().z;
                        m_vel.z = 0.0f;
                    }
                    else // axis == Axis::Y
                    {
                        m_pos.y = delta > 0.0f ? (float)y - 2.0f * m_hitBox.getHalfExtents().y
                                               : (float)(y + 1);
                        m_vel.y = 0.0f;
                        if (delta <= 0.0f)
                            m_onGround = true;
                    }
                }
    }
};