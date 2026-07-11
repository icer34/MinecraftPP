#include "camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, float aspectRatio)
    : m_pos(position),
      m_worldUp(0.0f, 1.0f, 0.0f),
      m_yaw(-90.0f),
      m_pitch(0.0f),
      m_fovDeg(70.0f),
      m_aspectRatio(aspectRatio),
      m_zNear(0.1f),
      m_zFar(500.0f),
      m_speed(4.5f),
      m_sens(0.1f)
{
    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const { return glm::lookAt(m_pos, m_pos + m_front, m_up); }

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(m_fovDeg), m_aspectRatio, m_zNear, m_zFar);
}

void Camera::move(glm::vec3 direction, float deltaTime)
{
    m_pos += direction * m_speed * deltaTime;
}

void Camera::rotate(float xOffset, float yOffset)
{
    m_yaw += xOffset * m_sens;
    m_pitch += yOffset * m_sens;

    // avoid camera flip
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::updateVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}