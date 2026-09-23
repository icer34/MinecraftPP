/**
 * @file camera.h
 * @brief First-person perspective camera.
 */

#pragma once

#include <glm/glm.hpp>

/**
 * @brief First-person camera driven by yaw/pitch angles, with a perspective projection.
 *
 * Pitch is clamped to [-89, 89] degrees so the camera can never flip over. The FOV, the zoom
 * speed and the mouse sensitivity are exposed in the settings menu.
 */
class Camera
{
public:
    /**
     * @param position initial world position of the camera
     */
    Camera(glm::vec3 position);

    /** @brief Moves the camera by `delta`, in world space. */
    void move(glm::vec3 delta);
    /** @brief Places the camera at `pos`, in world space. */
    void setPos(glm::vec3 pos) { _pos = pos; }
    /** @brief Sets the width / height ratio used by the projection matrix. */
    void setAspectRatio(float aspectRatio) { _aspectRatio = aspectRatio; }

    /**
     * @brief Rotates the camera from a mouse movement.
     *
     * @param xOffset horizontal mouse movement, scaled by the sensitivity and added to the yaw
     * @param yOffset vertical mouse movement, scaled by the sensitivity and added to the pitch
     */
    void rotate(float xOffset, float yOffset);

    /**
     * @brief Changes the FOV from a scroll wheel offset.
     *
     * A positive offset (scroll up) zooms in, i.e. narrows the FOV. The FOV is clamped
     * between the minimum and maximum FOV.
     */
    void zoom(float scrollOffset);

    /** @brief World position of the camera. */
    glm::vec3 getPos() const { return _pos; }
    /** @brief Direction the camera is looking at (unit vector). */
    glm::vec3 getFront() const { return _front; }
    /** @brief Right vector of the camera (unit vector). */
    glm::vec3 getRight() const { return _right; }
    /** @brief Up vector of the camera (unit vector). */
    glm::vec3 getUp() const { return _up; }
    /** @brief Vertical field of view, in degrees. */
    float getFOV() const { return float(_fovDeg); }
    /** @brief Distance of the near clipping plane. */
    float getZNear() const { return _zNear; }
    /** @brief Distance of the far clipping plane. */
    float getZFar() const { return _zFar; }
    /** @brief Width / height ratio used by the projection matrix. */
    float getAspectRatio() const { return _aspectRatio; }

    // temporary, for the frustum-disappearing-on-fast-rotation debug session
    /** @brief Yaw angle, in degrees. */
    float getYaw() const { return _yaw; }
    /** @brief Pitch angle, in degrees. */
    float getPitch() const { return _pitch; }

    /** @brief World-to-view matrix. */
    glm::mat4 getViewMatrix() const;
    /** @brief Perspective projection matrix. */
    glm::mat4 getProjectionMatrix() const;

private:
    void updateVectors();

    glm::vec3 _pos;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;

    float _yaw;
    float _pitch;

    float _zNear;
    float _zFar;
    int _fovDeg = 70;
    float _aspectRatio = 1.0f;

    float _sens;

    int MIN_FOV = 10;
    int MAX_FOV = 130; // default FOV, also the "fully zoomed out" value
    float ZOOM_STEP = 3.0f;
};