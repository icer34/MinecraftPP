/**
 * @file player.h
 * @brief The player entity and the per-frame input it consumes.
 */

#pragma once

#include "entity.h"

#include "graphics/camera.h"

/**
 * @brief Input gathered by the game for one frame, handed to Player::consumeInput().
 */
struct InputData
{
    glm::vec3 move = glm::vec3(0.0f); ///< Desired horizontal movement direction, in world space.
    bool jump = false;                ///< True while the jump key is held.
    float mouseDx = 0.0f;             ///< Horizontal mouse movement since the last frame.
    float mouseDy = 0.0f;             ///< Vertical mouse movement since the last frame.
    float scroll = 0.0f;              ///< Scroll wheel offset since the last frame (zoom).
};

/**
 * @brief The entity controlled by the user, with a first-person camera at eye height.
 *
 * Walks with gravity and jumping, collides with solid blocks, and exposes its tuning values
 * (reach, speeds...) in the settings menu.
 */
class Player : public Entity
{
public:
    /**
     * @param pos initial position of the player's feet
     */
    Player(glm::vec3 pos);

    /**
     * @brief Applies the last consumed input, gravity and jumping, moves the player with
     * collisions and places the camera at eye height.
     */
    void update(float dt, World &world) override;

    /**
     * @brief Stores the movement input for the next update() and applies the camera
     * rotation and zoom immediately.
     */
    void consumeInput(const InputData &inputData);

    /** @brief Direction the player is looking at (unit vector). */
    glm::vec3 getFront() const;
    /** @brief Right vector of the player's camera (unit vector). */
    glm::vec3 getRight() const;
    /** @brief Position of the player's feet. */
    glm::vec3 getPos() const;
    /** @brief Maximum distance, in blocks, at which the player can break or place blocks. */
    float getReach() const;
    /** @brief The player's first-person camera. */
    Camera &getCam();

private:
    // inventory, effects, gamemode, ...
    float _reach;
    float _walkSpeed;
    float _flySpeed;
    float _gravity;
    float _jumpVel;

    glm::vec3 _moveInput;
    bool _jumpPressed;

    Camera _cam;
    float _eyeHeight;
};