#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <yaml-cpp/yaml.h>

#include "core/math/math.h"

namespace sky
{
/**
 * @brief Represents a 3D transformation including position, rotation (as a quaternion), and scale.
 * Provides methods to manipulate the transformation and calculate the model matrix.
 */
class Transform
{
  public:
    Transform();
    /**
     * @brief Constructor to initialize the transform with specific position, rotation, and scale.
     * @param position The initial position of the transform.
     * @param rotation The initial rotation of the transform in Euler angles (degrees).
     * @param scale The initial scale of the transform.
     */
    // Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
    ~Transform() = default;

    /**
     * @brief Sets the position of the transform.
     * @param x X-coordinate of the position.
     * @param y Y-coordinate of the position.
     * @param z Z-coordinate of the position.
     */
    SKY_INLINE void setPosition(float x, float y, float z)
    {
        m_position.x = x;
        m_position.y = y;
        m_position.z = z;
    }
    /**
     * @brief Sets the position of the transform.
     * @param position The new position as a glm::vec3.
     */
    SKY_INLINE void setPosition(const glm::vec3 &position) { m_position = position; }
    /**
     * @brief Gets the current position of the transform.
     * @return The current position as a glm::vec3.
     */

    SKY_INLINE glm::vec3 &getPosition() { return m_position; }
    /**
     * @brief Gets the current position of the transform (const version).
     * @return The current position as a const glm::vec3&.
     */
    SKY_INLINE const glm::vec3 &getPosition() const { return m_position; }

    /**
     * @brief Sets the rotation of the transform using Euler angles in radians.
     * @param rotation The new rotation as a glm::vec3 in radians.
     */
    SKY_INLINE void setRotation(float x, float y, float z) { m_rotationQuat = glm::quat(glm::vec3(x, y, z)); }
    /**
     * @brief Sets the rotation of the transform using Euler angles in degrees.
     * @param rotation The new rotation as a glm::vec3 in degrees.
     */
    SKY_INLINE void setRotationDegrees(float x, float y, float z)
    {
        m_rotationQuat = glm::quat(glm::radians(glm::vec3(x, y, z)));
    }

    /**
     * @brief Gets the current rotation of the transform in Euler angles radians.
     * @return The current rotation as a glm::vec3 in radians.
     */
    SKY_INLINE void setRotation(const glm::vec3 &rotation) { m_rotationQuat = rotation; }

    /**
     * @brief Gets the current rotation of the transform in Euler angles (degrees).
     * @return The current rotation as a glm::vec3 in degrees.
     */
    SKY_INLINE void setRotationDegrees(const glm::vec3 &rotation)
    {
        m_rotationQuat = glm::quat(glm::radians(rotation));
    }
    /**
     * @brief Set the current rotation of the transform in Euler angles (degrees).
     * @return The current rotation as a glm::vec3 in degrees.
     */
    SKY_INLINE void setRotationDegrees(const glm::quat &rotation) { m_rotationQuat = rotation; }
    /**
     * @brief Gets the current rotation of the transform in radians.
     * @return The current rotation as a glm::vec3 in radians.
     */
    SKY_INLINE glm::vec3 getRotation() const { return glm::eulerAngles(m_rotationQuat); }
    /**
     * @brief Gets the current rotation of the transform in Euler angles (degrees).
     * @return The current rotation as a glm::vec3 in degrees.
     */
    SKY_INLINE glm::vec3 getRotationDegrees() const { return glm::degrees(glm::eulerAngles(m_rotationQuat)); }

    /**
     * @brief Sets the rotation of the transform using a quaternion.
     * @param quaternion The new rotation as a glm::quat.
     */
    SKY_INLINE void setRotation(const glm::quat &quaternion) { m_rotationQuat = quaternion; }
    /**
     * @brief Gets the current rotation of the transform as a quaternion.
     * @return The current rotation as a glm::quat.
     */
    SKY_INLINE glm::quat &getRotationQuaternion() { return m_rotationQuat; }
    /**
     * @brief Gets the current rotation of the transform as a quaternion (const version).
     * @return The current rotation as a const glm::quat&.
     */
    SKY_INLINE const glm::quat &getRotationQuaternion() const { return m_rotationQuat; }

    /**
     * @brief Sets the scale of the transform.
     * @param x Scale along the X-axis.
     * @param y Scale along the Y-axis.
     * @param z Scale along the Z-axis.
     */
    SKY_INLINE void setScale(float x, float y, float z)
    {
        m_scale.x = x;
        m_scale.y = y;
        m_scale.z = z;
    }

    /**
     * @brief Sets the scale of the transform.
     * @param scale The new scale as a glm::vec3.
     */
    SKY_INLINE void setScale(const glm::vec3 &scale) { m_scale = scale; }

    /**
     * @brief Gets the current scale of the transform.
     * @return The current scale as a glm::vec3.
     */
    SKY_INLINE glm::vec3 &getScale() { return m_scale; }

    /**
     * @brief Gets the current scale of the transform (const version).
     * @return The current scale as a const glm::vec3&.
     */
    SKY_INLINE const glm::vec3 &getScale() const { return m_scale; }

    /**
     * @brief Rotates the transform by the specified Euler angles in degrees.
     * @param x Rotation around the X-axis in degrees.
     * @param y Rotation around the Y-axis in degrees.
     * @param z Rotation around the Z-axis in degrees.
     */
    SKY_INLINE void rotate(float x, float y, float z) { rotate(glm::quat(glm::vec3(x, y, z))); }
    /**
     * @brief Rotates the transform by the specified Euler angles in degrees.
     * @param rotation The rotation to apply as a glm::vec3 in degrees.
     */
    SKY_INLINE void rotate(const glm::vec3 &rotation) { rotate(glm::quat(rotation)); }

    SKY_INLINE void rotateDegrees(const glm::vec3 &rotation) { rotate(glm::quat(glm::radians(rotation))); }

    /**
     * @brief Rotates the transform by the specified quaternion.
     * @param quaternion The quaternion representing the rotation to apply.
     */
    SKY_INLINE void rotate(const glm::quat &quaternion)
    {
        m_rotationQuat = glm::normalize(m_rotationQuat * quaternion);
    }

    /**
     * @brief Moves the transform by the specified amount.
     * @param x Movement along the X-axis.
     * @param y Movement along the Y-axis.
     * @param z Movement along the Z-axis.
     */
    SKY_INLINE void move(float x, float y, float z)
    {
        m_position.x += x;
        m_position.y += y;
        m_position.z += z;
    }

    /**
     * @brief Moves the transform by the specified amount.
     * @param position The movement to apply as a glm::vec3.
     */
    SKY_INLINE void move(const glm::vec3 &position) { m_position += position; }

    /**
     * @brief Sets the forward direction of the transform.
     * @param direction The new forward direction.
     * @param forwardDirection The reference forward direction, default is (0, 0, 1).
     */
    SKY_INLINE void setDirection(const glm::vec3 &direction, const glm::vec3 &upDirection = glm::vec3(0.f, 1.f, 0.f))
    {
        m_rotationQuat = glm::quatLookAt(glm::normalize(direction), upDirection); // Assumes up is always (0,1,0)
    }

    /**
     * @brief Gets the current forward direction of the transform.
     * @param forwardDirection The reference forward direction, default is (0, 0, 1, 1).
     * @return The current forward direction as a glm::vec3.
     */
    SKY_INLINE glm::vec3 getForwardDirection(const glm::vec4 &worldForwardDirection = glm::vec4(0.f, 0.f, -1.f,1.f)) const
    {
        // TIP: Not sure if there should be normilize !
        return glm::normalize(getRotationQuaternion() * worldForwardDirection);
    }

    /**
     * @brief Calculates the pitch angle (rotation around the X-axis) from the current quaternion rotation.
     * @return The pitch angle in radians.
     */
    SKY_INLINE float getPitch() const
    {
        // Calculate the pitch angle in radians
        return glm::pitch(m_rotationQuat);
    }

    /**
     * @brief Calculates the yaw angle (rotation around the Y-axis) from the current quaternion rotation.
     * @return The yaw angle in radians.
     */
    SKY_INLINE float getYaw() const
    {
        // Calculate the roll angle in radians
        return glm::yaw(m_rotationQuat);
    }
    /**
     * @brief Calculates the roll angle (rotation around the Z-axis) from the current quaternion rotation.
     * @return The roll angle in radians.
     */
    SKY_INLINE float getRoll() const
    {
        // Calculate the yaw angle in radians
        return glm::roll(m_rotationQuat);
    }

    /**
     * @brief Gets the model matrix representing the transform.
     * @return The model matrix as a glm::mat4.
     */
    SKY_INLINE glm::mat4 getModelMatrix() const
    {
        return glm::translate(glm::mat4(1.f), m_position) * glm::toMat4(m_rotationQuat) *
               glm::scale(glm::mat4(1.f), m_scale);
    }

    /**
     * @brief Sets the transform from a given transformation matrix.
     * @param matrix The transformation matrix to decompose.
     */
    SKY_INLINE void setTransformMatrix(const glm::mat4 &matrix)
    {
        math::DecomposeMatrix(matrix, m_position, m_rotationQuat, m_scale);
    }

    /**
     * @brief Creates a transform from a given transformation matrix.
     * @param matrix The transformation matrix to decompose.
     * @return A Transform object with the decomposed position, rotation, and scale.
     */
    SKY_INLINE static Transform getTransformFromMatrix(const glm::mat4 &matrix)
    {
        Transform transform;
        math::DecomposeMatrix(matrix, transform.m_position, transform.m_rotationQuat, transform.m_scale);
        return transform;
    }

  public:
    void serialize(YAML::Emitter &out);
    void deserialize(YAML::detail::iterator_value entity);

  private:
    glm::vec3 m_position;     ///< The position of the transform.
    glm::quat m_rotationQuat; ///< The rotation of the transform stored as a quaternion.
    glm::vec3 m_scale;        ///< The scale of the transform.
};
} // namespace sky