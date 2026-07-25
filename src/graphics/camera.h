#pragma once

#include <glm/glm.hpp>

class Camera
{
  public:
    Camera(glm::vec3 position);

    void move(glm::vec3 delta);
    void setPos(glm::vec3 pos) { m_pos = pos; }
    void rotate(float xOffset, float yOffset);
    void zoom(float scrollOffset);

    glm::vec3 getPos() const { return m_pos; }
    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getRight() const { return m_right; }
    glm::vec3 getUp() const { return m_up; }
    float getFOV() const { return m_fovDeg; }
    float getZNear() const { return m_zNear; }
    float getZFar() const { return m_zFar; }

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

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
    float m_fovDeg;

    float m_sens;

    static constexpr float MIN_FOV = 10.0f;
    static constexpr float MAX_FOV = 70.0f; // default FOV, also the "fully zoomed out" value
    static constexpr float ZOOM_STEP = 3.0f;
};