#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include "camera_frustum.h"
#include "camera.h"

#include <skypch.h>

namespace sky
{
class PerspectiveCamera : public Camera
{
  public:
    PerspectiveCamera();
    /**
     * @brief Constructor for PerspectiveCamera.
     * @param position Initial position of the camera.
     * @param fov Field of view in degrees.
     * @param aspect Aspect ratio of the camera.
     * @param zNear Near clipping plane distance.
     * @param zFar Far clipping plane distance.
     */
    PerspectiveCamera(const glm::vec3 &position, float fov, float aspect, float zNear, float zFar);
    virtual ~PerspectiveCamera() = default;

    PerspectiveCamera &operator=(const PerspectiveCamera &camera) { *this = camera; }

    /**
     * @brief Get the view matrix.
     * @return View matrix.
     */
    SKY_INLINE glm::mat4 getView() const { return glm::lookAt(m_position, m_position + m_forward, m_up); }

    /**
     * @brief Get the projection matrix.
     * @return Projection matrix.
     */
    SKY_INLINE const glm::mat4 &getProjection() const { return m_perpective; }

    /**
     * @brief Get the combined view-projection matrix.
     * @return View-projection matrix.
     */
    SKY_INLINE glm::mat4 getViewProjection() const override { return m_perpective * getView(); }

    /**
     * @brief Get a modifiable reference to the projection matrix.
     * @return Reference to the projection matrix.
     */
    SKY_INLINE glm::mat4 &getProjection() { return m_perpective; }

    /**
     * @brief Get a modifiable reference to the forward direction.
     * @return Reference to the forward direction vector.
     */
    SKY_INLINE glm::vec3 &getForwardDirection() { return m_forward; }

    /**
     * @brief Get a constant reference to the forward direction.
     * @return Constant reference to the forward direction vector.
     */
    SKY_INLINE const glm::vec3 &getForwardDirection() const { return m_forward; }

    /**
     * @brief Get a modifiable reference to the up direction.
     * @return Reference to the up direction vector.
     */
    SKY_INLINE glm::vec3 &getUpDirection() { return m_up; }

    /**
     * @brief Get a constant reference to the up direction.
     * @return Constant reference to the up direction vector.
     */
    SKY_INLINE const glm::vec3 &getUpDirection() const { return m_up; }

    /**
     * @brief Get a modifiable reference to the position.
     * @return Reference to the position vector.
     */
    SKY_INLINE glm::vec3 &getPosition() { return m_position; }

    /**
     * @brief Get a constant reference to the position.
     * @return Constant reference to the position vector.
     */
    SKY_INLINE const glm::vec3 &getPosition() const { return m_position; }

    /**
     * @brief Set the camera view using a view matrix.
     * @param view The view matrix to set.
     */
    SKY_INLINE void setView(const glm::mat4 &view)
    {
        glm::mat4 matrix = getProjection() * glm::inverse(view);
        // glm::vec2 right	= glm::vec3(matrix[0][0], matrix[1][0], matrix[2][0]);
        m_forward = glm::vec3(matrix[0][2], matrix[1][2], matrix[2][2]);
        m_up = glm::vec3(matrix[0][1], matrix[1][1], matrix[2][1]);
        m_position = glm::vec3(view[3][0], view[3][1], view[3][2]);
    }

    /**
     * @brief Get the field of view (FOV).
     * @return FOV in degrees.
     */
    SKY_INLINE const float &getFov() const { return m_fov; }

    /**
     * @brief Get a modifiable reference to the field of view (FOV).
     * @return Reference to the FOV in degrees.
     */
    SKY_INLINE float &getFov() { return m_fov; }

    /**
     * @brief Get the aspect ratio.
     * @return Aspect ratio.
     */
    SKY_INLINE const float &getAspect() const { return m_aspect; }

    /**
     * @brief Get a modifiable reference to the aspect ratio.
     * @return Reference to the aspect ratio.
     */
    SKY_INLINE float &getAspect() { return m_aspect; }

    /**
     * @brief Get the near clipping plane distance.
     * @return Near clipping plane distance.
     */
    SKY_INLINE const float &getNear() const { return m_zNear; }

    /**
     * @brief Get a modifiable reference to the near clipping plane distance.
     * @return Reference to the near clipping plane distance.
     */
    SKY_INLINE float &getNear() { return m_zNear; }

    /**
     * @brief Get the far clipping plane distance.
     * @return Far clipping plane distance.
     */
    SKY_INLINE const float &getFar() const { return m_zFar; }

    /**
     * @brief Get a modifiable reference to the far clipping plane distance.
     * @return Reference to the far clipping plane distance.
     */
    SKY_INLINE float &getFar() { return m_zFar; }

    /**
     * @brief Set the camera position.
     * @param x X coordinate of the position.
     * @param y Y coordinate of the position.
     * @param z Z coordinate of the position.
     */
    SKY_INLINE void setPosition(float x, float y, float z)
    {
        m_position.x = x;
        m_position.y = y;
        m_position.z = z;
    }

    /**
     * @brief Set the camera position.
     * @param position The new position of the camera.
     */
    SKY_INLINE void setPosition(const glm::vec3 &position) { m_position = position; }

    /**
     * @brief Set the forward direction of the camera.
     * @param x X component of the forward direction.
     * @param y Y component of the forward direction.
     * @param z Z component of the forward direction.
     */
    SKY_INLINE void setForwardDirection(float x, float y, float z)
    {
        m_forward.x = x;
        m_forward.y = y;
        m_forward.z = z;
    }

    /**
     * @brief Set the forward direction of the camera.
     * @param direction The new forward direction.
     */
    SKY_INLINE void setForwardDirection(const glm::vec3 &direction) { m_forward = direction; }

    /**
     * @brief Move the camera forward by a given value.
     * @param value Distance to move forward.
     */
    SKY_INLINE void moveForward(float value) { m_position += m_forward * value; }

    /**
     * @brief Move the camera backward by a given value.
     * @param value Distance to move backward.
     */
    SKY_INLINE void moveBackward(float value) { m_position -= m_forward * value; }

    /**
     * @brief Move the camera to the right by a given value.
     * @param value Distance to move right.
     */
    SKY_INLINE void moveRight(float value) { m_position -= glm::cross(m_up, m_forward) * value; }

    /**
     * @brief Move the camera to the left by a given value.
     * @param value Distance to move left.
     */
    SKY_INLINE void moveLeft(float value) { m_position += glm::cross(m_up, m_forward) * value; }

    /**
     * @brief Set the aspect ratio of the camera.
     * @param aspect New aspect ratio.
     */
    SKY_INLINE void setAspect(float aspect)
    {
        m_aspect = aspect;
        recalculatePerpective();
    }

    /**
     * @brief Set the field of view (FOV) of the camera.
     * @param fov New field of view in degrees.
     */
    SKY_INLINE void setFov(float fov)
    {
        m_fov = fov;
        recalculatePerpective();
    }

    /**
     * @brief Set the near clipping plane distance.
     * @param zNear New near clipping plane distance.
     */
    SKY_INLINE void setNear(float zNear)
    {
        m_zNear = zNear;
        recalculatePerpective();
    }

    /**
     * @brief Set the far clipping plane distance.
     * @param zFar New far clipping plane distance.
     */
    SKY_INLINE void setFar(float zFar)
    {
        m_zFar = zFar;
        recalculatePerpective();
    }

    /**
     * @brief Rotate the camera around the yaw axis.
     * @param angle Angle in radians to rotate around the yaw axis.
     */
    SKY_INLINE void rotateYaw(float angle)
    {
        // With counterclockwise issue
        // glm::mat4 rotation = glm::rotate(angle, UP);
        // Without counterclockwise issue
        glm::mat4 rotation = glm::rotate(angle, glm::dot(UP, m_up) < 0.f ? -UP : UP);
        m_forward = glm::vec3(glm::normalize(rotation * glm::vec4(m_forward, 0.f)));
        m_up = glm::vec3(glm::normalize(rotation * glm::vec4(m_up, 0.f)));
    }

    /**
     * @brief Rotate the camera around the pitch axis.
     * @param angle Angle in radians to rotate around the pitch axis.
     */
    SKY_INLINE void rotatePitch(float angle)
    {
        glm::vec3 right = glm::normalize(glm::cross(m_up, m_forward));
        m_forward = glm::normalize(glm::rotate(-angle, right) * glm::vec4(m_forward, 0.f));
        m_up = glm::normalize(glm::cross(m_forward, right));
    }

    /**
     * @brief Rotate the camera around the roll axis.
     * @param angle Angle in radians to rotate around the roll axis.
     */
    SKY_INLINE void rotateRoll(float angle)
    {
        m_up = glm::normalize(glm::rotate(-angle, m_forward) * glm::vec4(m_up, 0.f));
    }

    /**
     * @brief Rotate the camera around all three axes.
     * @param x Angle in radians to rotate around the yaw axis.
     * @param y Angle in radians to rotate around the pitch axis.
     * @param z Angle in radians to rotate around the roll axis.
     */
    SKY_INLINE void rotate(float x, float y, float z)
    {
        rotateYaw(-x);
        rotatePitch(-y);
        rotateRoll(z);
    }

    /**
     * @brief Rotate the camera around all three axes.
     * @param angle Vector containing angles in radians for yaw (x), pitch (y), and roll (z).
     */
    SKY_INLINE void rotate(const glm::vec3 &angle)
    {
        rotateYaw(-angle.x);
        rotatePitch(-angle.y);
        rotateRoll(angle.z);
    }

    /**
     * @brief Resize the camera perspective.
     * @param aspect New aspect ratio. If zero, keep the current aspect ratio.
     */
    SKY_INLINE void resize(float aspect = 0)
    {
        (aspect) ? m_aspect = aspect, recalculatePerpective() : recalculatePerpective();
    }

    /**
     * @brief Recalculate the camera perspective matrix.
     */
    SKY_INLINE void recalculatePerpective()
    {
        m_perpective = glm::perspective(glm::radians(m_fov), m_aspect, m_zNear, m_zFar);
        m_perpective[1][1] *= -1.0f; // Flip Y
    }

    /**
     * @brief Check if the camera is set as the primary camera.
     * @return True if the camera is primary, false otherwise.
     */
    SKY_INLINE bool isPrimary() const { return m_isPrimary; }

    /**
     * @brief Set the camera as primary or not.
     * @param isPrimary True to set the camera as primary.
     */
    SKY_INLINE void setPrimary(bool isPrimary) { m_isPrimary = isPrimary; }

    /**
     * @brief Get the yaw angle of the camera.
     * @return Yaw angle in radians.
     */
    SKY_INLINE float getYaw() const { return glm::atan(m_forward.x, m_forward.z); }

    /**
     * @brief Get the pitch angle of the camera.
     * @return pitch angle in radians.
     */
    SKY_INLINE float getPitch() const { return glm::asin(m_forward.y); }

    /**
     * @brief Get the pitch angle of the camera.
     * @return roll angle in radians.
     */
    SKY_INLINE float getRoll() const { return glm::atan(m_up.x, m_up.y); }

    /**
     * @brief Get the camera frustum.
     * @return CameraFrustum object representing the frustum.
     */
    SKY_INLINE CameraFrustum getCameraFrustum() const { return CameraFrustum(getViewProjection()); }

    /**
     * @brief Get camera render data.
     * @return RenderData structure containing camera data.
     */
    SKY_INLINE RenderData getRenderData() const override
    {
        return {getViewProjection(),
                getView(),
                getProjection(),
                getPosition(),
                getForwardDirection(), /*-m_perpective[3][2], m_perpective[2][2]*/
                getNear(),
                getFar()};
    }

  private:
    const glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f); // Y is up
    glm::mat4 m_perpective;
    glm::vec3 m_position, m_forward, m_up;
    float m_fov, m_aspect, m_zNear, m_zFar;
    // TODO: make it like component
    bool m_isPrimary = false;
};
#ifndef CAMERA_DATA_SIZE
#define CAMERA_DATA_SIZE (sizeof(Camera::RenderData))
#endif // !CAMERA_DATA_SIZE
} // namespace sky