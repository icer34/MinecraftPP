#include "player.h"

#include "settings_registry.h"
#include "world.h"

using glm::vec3;

Player::Player(vec3 pos)
    : m_cam(pos),
      Entity(pos, vec3(0.25f, 0.95f, 0.25f))
{
    m_reach = 4.0f;
    m_walkSpeed = 6.0f;
    m_flySpeed = 15.0f;
    m_gravity = 15.0f;
    m_jumpVel = 6.0f;
    m_eyeHeight = 1.7f;

    auto &reg = SettingsRegistry::instance();
    reg.addFloat(SettingCategory::Gameplay, "", "reach", &m_reach, 1.0f, 10.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "walk_speed", &m_walkSpeed, 3.0f, 30.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "fly_speed", &m_flySpeed, 10.0f, 100.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "gravity", &m_gravity, 10.0f, 20.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "jump_vel", &m_jumpVel, 1.0f, 15.0f);
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

void Player::consumeInput(const InputData &inputData)
{
    m_moveInput = inputData.move;
    m_jumpPressed = inputData.jump;

    m_cam.rotate(inputData.mouseDx, inputData.mouseDy);
    m_cam.zoom(inputData.scroll);
}

vec3 Player::getFront() const { return m_cam.getFront(); }

vec3 Player::getRight() const { return m_cam.getRight(); }

vec3 Player::getPos() const { return m_pos; }

float Player::getReach() const { return m_reach; }

Camera &Player::getCam() { return m_cam; }