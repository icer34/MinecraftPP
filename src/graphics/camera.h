#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera(glm::vec3 position);

    void move(glm::vec3 delta);
    void setPos(glm::vec3 pos) { m_pos = pos; }
    void setAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    void rotate(float xOffset, float yOffset);
    void zoom(float scrollOffset);

    glm::vec3 getPos() const { return m_pos; }
    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getRight() const { return m_right; }
    glm::vec3 getUp() const { return m_up; }
    float getFOV() const { return float(m_fovDeg); }
    float getZNear() const { return m_zNear; }
    float getZFar() const { return m_zFar; }
    float getAspectRatio() const { return m_aspectRatio; }

    // temporary, for the frustum-disappearing-on-fast-rotation debug session
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

private:
    void updateVectors();

    glm::vec3 m_pos;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_zNear;
    float m_zFar;
    int m_fovDeg = 70;
    float m_aspectRatio = 1.0f;

    float m_sens;

    int MIN_FOV = 10;
    int MAX_FOV = 130; // default FOV, also the "fully zoomed out" value
    float ZOOM_STEP = 3.0f;
};