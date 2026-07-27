#include "player.h"

#include "world.h"

using glm::vec3;

Player::Player(vec3 pos)
    : m_cam(pos),
      Entity(pos, vec3(0.25f, 0.95f, 0.25f))
{
    m_reach = 4.0f;
    m_walkSpeed = 6.0f;
    m_flySpeed = 10.0f;
    m_gravity = 15.0f;
    m_jumpVel = 6.0f;
    m_eyeHeight = 1.7f;
}

void Player::update(float dt, World &world)
{
    m_vel.x = m_moveInput.x * m_walkSpeed;
    m_vel.z = m_moveInput.z * m_walkSpeed;
    if (!m_onGround)
        m_vel.y -= m_gravity * dt;
    if (m_jumpPressed && m_onGround)
        m_vel.y = m_jumpVel;

    this->moveAndCollide(m_vel * dt, world);

    m_cam.setPos(m_pos + vec3(0.0f, m_eyeHeight, 0.0f));
}

void Player::setMoveInput(vec3 moveInput, bool jumpPressed)
{
    m_moveInput = moveInput;
    m_jumpPressed = jumpPressed;
}

void Player::rotateCam(float dx, float dy) { m_cam.rotate(dx, dy); }

void Player::zoomCam(float scroll) { m_cam.zoom(scroll); }

vec3 Player::getFront() const { return m_cam.getFront(); }

vec3 Player::getRight() const { return m_cam.getRight(); }

vec3 Player::getPos() const { return m_pos; }

Camera &Player::getCam() { return m_cam; }