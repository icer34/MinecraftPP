/**
 * @file entity.h
 * @brief Base class for moving objects that collide with the world.
 */

#pragma once

#include <glm/glm.hpp>

#include "game/world.h"
#include "util/aabb.h"
#include "util/directions.h"

/**
 * @brief Base class for anything that moves through the world and collides with solid blocks.
 *
 * Provides a position (the feet of the hitbox), a velocity, and axis-by-axis collision
 * resolution against the world's solid blocks.
 */
class Entity
{
public:
    /**
     * @param pos initial position of the entity's feet
     * @param hitboxHalfExtents half of the hitbox size on each axis
     */
    Entity(glm::vec3 pos, glm::vec3 hitboxHalfExtents)
        : _pos(pos),
          _vel(0.0f),
          _hitBox(hitboxHalfExtents),
          _onGround(false)
    {
    }

    virtual ~Entity() = default;

    /**
     * @brief Advances the entity by one frame.
     *
     * @param dt duration of the frame, in seconds
     * @param world world to collide with
     */
    virtual void update(float dt, World &world) = 0;

    /** @brief The entity's hitbox, relative to its position. */
    const AABB &getHitBox() const { return _hitBox; }

protected:
    glm::vec3 _pos; ///< Position of the entity's feet (bottom-center of the hitbox).
    glm::vec3 _vel; ///< Velocity, in blocks per second.

    AABB _hitBox; ///< Hitbox, anchored at _pos.

    bool _onGround; ///< True if the last vertical move ended on a solid block.

    /**
     * @brief Moves the entity by `delta` and pushes it out of any solid block it enters.
     *
     * Each axis is moved and resolved separately (X, then Z, then Y) so the entity slides
     * along walls instead of getting stuck in corners. The velocity is zeroed on each axis
     * where a collision happens, and _onGround is updated from the vertical move.
     *
     * @param delta movement for this frame, in blocks
     * @param world world to collide with
     */
    void moveAndCollide(glm::vec3 delta, World &world)
    {
        //*resolve collisions for each axis independantly to avoid being stuck in a corner
        _pos.x += delta.x;
        resolveAxisCollision(Axis::X, delta.x, world);

        _pos.z += delta.z;
        resolveAxisCollision(Axis::Z, delta.z, world);

        _onGround = false;
        _pos.y += delta.y;
        resolveAxisCollision(Axis::Y, delta.y, world);
    }

private:
    void resolveAxisCollision(Axis axis, float delta, World &world)
    {
        glm::vec3 min = _hitBox.getMin(_pos);
        glm::vec3 max = _hitBox.getMax(_pos);

        for (int x = (int)floor(min.x); x < (int)ceil(max.x); x++)
            for (int y = (int)floor(min.y); y < (int)ceil(max.y); y++)
                for (int z = (int)floor(min.z); z < (int)ceil(max.z); z++)
                {
                    if (!world.isBlockSolid(glm::vec3(x, y, z)))
                        continue;

                    if (axis == Axis::X)
                    {
                        _pos.x = delta > 0.0f ? (float)x - _hitBox.getHalfExtents().x
                                              : (float)(x + 1) + _hitBox.getHalfExtents().x;
                        _vel.x = 0.0f;
                    }
                    else if (axis == Axis::Z)
                    {
                        _pos.z = delta > 0.0f ? (float)z - _hitBox.getHalfExtents().z
                                              : (float)(z + 1) + _hitBox.getHalfExtents().z;
                        _vel.z = 0.0f;
                    }
                    else // axis == Axis::Y
                    {
                        _pos.y = delta > 0.0f ? (float)y - 2.0f * _hitBox.getHalfExtents().y
                                              : (float)(y + 1);
                        _vel.y = 0.0f;
                        if (delta <= 0.0f)
                            _onGround = true;
                    }
                }
    }
};