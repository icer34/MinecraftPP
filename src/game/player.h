#pragma once

#include "entity.h"

#include "graphics/camera.h"

struct InputData
{
    glm::vec3 move = glm::vec3(0.0f);
    bool jump = false;
    float mouseDx = 0.0f, mouseDy = 0.0f;
    float scroll = 0.0f;
};

class Player : public Entity
{
public:
    Player(glm::vec3 pos);

    void update(float dt, World &world) override;

    void consumeInput(const InputData &inputData);

    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getPos() const;
    float getReach() const;
    Camera &getCam();

private:
    // inventory, effects, gamemode, ...
    float m_reach;
    float m_walkSpeed;
    float m_flySpeed;
    float m_gravity;
    float m_jumpVel;

    glm::vec3 m_moveInput;
    bool m_jumpPressed;

    Camera m_cam;
    float m_eyeHeight;
};