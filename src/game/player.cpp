#include "player.h"

#include "settings_registry.h"
#include "world.h"

using glm::vec3;

Player::Player(vec3 pos)
    : _cam(pos),
      Entity(pos, vec3(0.25f, 0.95f, 0.25f))
{
    _reach = 4.0f;
    _walkSpeed = 6.0f;
    _flySpeed = 15.0f;
    _gravity = 15.0f;
    _jumpVel = 6.0f;
    _eyeHeight = 1.7f;

    auto &reg = SettingsRegistry::instance();
    reg.addFloat(SettingCategory::Gameplay, "", "reach", &_reach, 1.0f, 10.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "walk_speed", &_walkSpeed, 3.0f, 30.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "fly_speed", &_flySpeed, 10.0f, 100.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "gravity", &_gravity, 10.0f, 20.0f);
    reg.addFloat(SettingCategory::Gameplay, "", "jump_vel", &_jumpVel, 1.0f, 15.0f);
}

void Player::update(float dt, World &world)
{
    _vel.x = _moveInput.x * _walkSpeed;
    _vel.z = _moveInput.z * _walkSpeed;
    if (!_onGround)
        _vel.y -= _gravity * dt;
    if (_jumpPressed && _onGround)
        _vel.y = _jumpVel;

    this->moveAndCollide(_vel * dt, world);

    _cam.setPos(_pos + vec3(0.0f, _eyeHeight, 0.0f));
}

void Player::consumeInput(const InputData &inputData)
{
    _moveInput = inputData.move;
    _jumpPressed = inputData.jump;

    _cam.rotate(inputData.mouseDx, inputData.mouseDy);
    _cam.zoom(inputData.scroll);
}

vec3 Player::getFront() const { return _cam.getFront(); }

vec3 Player::getRight() const { return _cam.getRight(); }

vec3 Player::getPos() const { return _pos; }

float Player::getReach() const { return _reach; }

Camera &Player::getCam() { return _cam; }