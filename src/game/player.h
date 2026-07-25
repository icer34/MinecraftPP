#pragma once

#include "entity.h"

#include "graphics/camera.h"

class Player : public Entity
{
  public:
    Player(glm::vec3 pos);

    void update(float dt, World &world) override;

    void rotateCam(float dx, float dy);
    void zoomCam(float scroll);
    void setMoveInput(glm::vec3 moveInput, bool jumpPressed);

    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getPos() const;
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