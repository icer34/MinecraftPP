#include "camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "game/settings_registry.h"

using glm::mat4;
using glm::vec3;

Camera::Camera(vec3 position)
    : _pos(position),
      _worldUp(0.0f, 1.0f, 0.0f),
      _yaw(-90.0f),
      _pitch(0.0f),
      _fovDeg(70.0f),
      _zNear(0.1f),
      _zFar(500.0f),
      _sens(0.1f)
{
    auto &reg = SettingsRegistry::instance();

    reg.addFloat(SettingCategory::Graphics, "", "Zoom Speed", &ZOOM_STEP, 1.0f, 10.0f);
    reg.addInt(SettingCategory::Graphics, "", "fov", &_fovDeg, MIN_FOV, MAX_FOV);
    reg.addFloat(SettingCategory::Controls, "", "Camera Speed", &_sens, 0.01f, 0.5f);

    updateVectors();
}

mat4 Camera::getViewMatrix() const { return glm::lookAt(_pos, _pos + _front, _up); }

mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(float(_fovDeg)), _aspectRatio, _zNear, _zFar);
}

void Camera::move(vec3 delta) { _pos += delta; }

void Camera::rotate(float xOffset, float yOffset)
{
    _yaw += xOffset * _sens;
    // clamp yaw so it doesnt accumulate to infinity
    _yaw = std::fmod(_yaw, 360.0f);
    _pitch += yOffset * _sens;

    // avoid camera flip
    _pitch = std::clamp(_pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::zoom(float scrollOffset)
{
    // scroll up (positive offset) zooms in -> smaller FOV
    _fovDeg -= scrollOffset * ZOOM_STEP;
    _fovDeg = std::clamp(_fovDeg, MIN_FOV, MAX_FOV);
}

void Camera::updateVectors()
{
    vec3 front;
    front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    front.y = sin(glm::radians(_pitch));
    front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

    _front = glm::normalize(front);
    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}