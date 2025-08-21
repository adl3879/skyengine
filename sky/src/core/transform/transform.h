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
class Transform
{
  public:
    Transform();
    ~Transform() = default;

    SKY_INLINE void setPosition(float x, float y, float z)
    {
        m_position.x = x;
        m_position.y = y;
        m_position.z = z;
    }
    
    SKY_INLINE void setPosition(const glm::vec3 &position) { m_position = position; }

    SKY_INLINE glm::vec3 &getPosition() { return m_position; }
    
    SKY_INLINE const glm::vec3 &getPosition() const { return m_position; }

    SKY_INLINE void setRotation(float x, float y, float z) 
    { 
        m_rotationQuat = glm::normalize(glm::quat(glm::vec3(x, y, z)));
    }
    
    SKY_INLINE void setRotationDegrees(float x, float y, float z)
    {
        glm::vec3 radians = glm::radians(glm::vec3(x, y, z));
        m_rotationQuat = glm::normalize(glm::quat(radians));
    }

    SKY_INLINE void setRotation(const glm::vec3 &rotation) 
    { 
        m_rotationQuat = glm::normalize(glm::quat(rotation)); 
    }

    SKY_INLINE void setRotationDegrees(const glm::vec3 &rotation)
    {
        glm::vec3 radians = glm::radians(rotation);
        m_rotationQuat = glm::normalize(glm::quat(radians));
    }
    
    SKY_INLINE void setRotationDegrees(const glm::quat &rotation) 
    { 
        m_rotationQuat = glm::normalize(rotation); 
    }
    
    SKY_INLINE glm::vec3 getRotation() const 
    { 
        return glm::eulerAngles(glm::normalize(m_rotationQuat)); 
    }
    
    SKY_INLINE glm::vec3 getRotationDegrees() const 
    { 
        return glm::degrees(glm::eulerAngles(glm::normalize(m_rotationQuat))); 
    }

    SKY_INLINE glm::vec3 getRotationSafe() const 
    { 
        glm::vec3 euler = glm::eulerAngles(glm::normalize(m_rotationQuat));
        
        euler.x = glm::mod(euler.x + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
        euler.y = glm::mod(euler.y + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
        euler.z = glm::mod(euler.z + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
        
        return euler;
    }
    
    SKY_INLINE glm::vec3 getRotationDegreesSafe() const 
    { 
        return glm::degrees(getRotationSafe());
    }

    SKY_INLINE void setRotation(const glm::quat &quaternion) 
    { 
        m_rotationQuat = glm::normalize(quaternion); 
    }
    
    SKY_INLINE glm::quat &getRotationQuaternion() { return m_rotationQuat; }
    
    SKY_INLINE const glm::quat &getRotationQuaternion() const { return m_rotationQuat; }

    SKY_INLINE void setScale(float x, float y, float z)
    {
        m_scale.x = x;
        m_scale.y = y;
        m_scale.z = z;
    }

    SKY_INLINE void setScale(const glm::vec3 &scale) { m_scale = scale; }

    SKY_INLINE glm::vec3 &getScale() { return m_scale; }

    SKY_INLINE const glm::vec3 &getScale() const { return m_scale; }

    SKY_INLINE void rotate(float x, float y, float z) 
    { 
        rotate(glm::quat(glm::vec3(x, y, z))); 
    }
    
    SKY_INLINE void rotate(const glm::vec3 &rotation) 
    { 
        rotate(glm::quat(rotation)); 
    }

    SKY_INLINE void rotateDegrees(const glm::vec3 &rotation) 
    { 
        rotate(glm::quat(glm::radians(rotation))); 
    }

    SKY_INLINE void rotate(const glm::quat &quaternion)
    {
        m_rotationQuat = glm::normalize(m_rotationQuat * quaternion);
    }

    SKY_INLINE void move(float x, float y, float z)
    {
        m_position.x += x;
        m_position.y += y;
        m_position.z += z;
    }

    SKY_INLINE void move(const glm::vec3 &position) { m_position += position; }

    SKY_INLINE void setDirection(const glm::vec3 &direction, const glm::vec3 &upDirection = glm::vec3(0.f, 1.f, 0.f))
    {
        glm::vec3 normalizedDir = glm::normalize(direction);
        glm::vec3 normalizedUp = glm::normalize(upDirection);
        
        glm::vec3 right = glm::normalize(glm::cross(normalizedDir, normalizedUp));
        glm::vec3 up = glm::cross(right, normalizedDir);
        
        glm::mat3 rotationMatrix;
        rotationMatrix[0] = right;
        rotationMatrix[1] = up;
        rotationMatrix[2] = -normalizedDir;
        
        m_rotationQuat = glm::normalize(glm::quat_cast(rotationMatrix));
    }

    SKY_INLINE glm::vec3 getForwardDirection() const
    {
        glm::vec3 forward = glm::normalize(m_rotationQuat) * glm::vec3(0.0f, 0.0f, -1.0f);
        return glm::normalize(forward);
    }

    SKY_INLINE glm::vec3 getForwardDirection(const glm::vec4 &worldForwardDirection) const
    {
        glm::vec3 forward = glm::vec3(worldForwardDirection);
        return glm::normalize(glm::normalize(m_rotationQuat) * forward);
    }

    SKY_INLINE glm::vec3 getRightDirection() const
    {
        glm::vec3 right = glm::normalize(m_rotationQuat) * glm::vec3(1.0f, 0.0f, 0.0f);
        return glm::normalize(right);
    }

    SKY_INLINE glm::vec3 getUpDirection() const
    {
        glm::vec3 up = glm::normalize(m_rotationQuat) * glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::normalize(up);
    }

    SKY_INLINE float getPitch() const
    {
        return glm::pitch(glm::normalize(m_rotationQuat));
    }

    SKY_INLINE float getYaw() const
    {
        return glm::yaw(glm::normalize(m_rotationQuat));
    }
    
    SKY_INLINE float getRoll() const
    {
        return glm::roll(glm::normalize(m_rotationQuat));
    }

    SKY_INLINE glm::mat4 getModelMatrix() const
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), m_position);
        glm::mat4 rotation = glm::toMat4(glm::normalize(m_rotationQuat));
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_scale);
        
        return translation * rotation * scale;
    }

    SKY_INLINE glm::mat4 getWorldMatrix() const
    {
        return m_worldMatrix;
    }

    SKY_INLINE void setTransformMatrix(const glm::mat4 &matrix)
    {
        math::DecomposeMatrix(matrix, m_position, m_rotationQuat, m_scale);
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }

    SKY_INLINE static Transform getTransformFromMatrix(const glm::mat4 &matrix)
    {
        Transform transform;
        math::DecomposeMatrix(matrix, transform.m_position, transform.m_rotationQuat, transform.m_scale);
        transform.m_rotationQuat = glm::normalize(transform.m_rotationQuat);
        return transform;
    }

    SKY_INLINE void normalizeRotation()
    {
        m_rotationQuat = glm::normalize(m_rotationQuat);
    }

    SKY_INLINE void ensurePositiveScale()
    {
        m_scale.x = glm::abs(m_scale.x);
        m_scale.y = glm::abs(m_scale.y);
        m_scale.z = glm::abs(m_scale.z);
    }

    SKY_INLINE void setWorldFromMatrix(const glm::mat4& matrix)
    {
        m_worldMatrix = matrix;
        math::DecomposeMatrix(m_worldMatrix, m_worldPosition, m_worldRotation, m_worldScale);
    }

    void debugPrint() const
    {
        glm::vec3 pos = getPosition();
        glm::vec3 rot = getRotationDegreesSafe();
        glm::vec3 scale = getScale();
        glm::vec3 forward = getForwardDirection();
        glm::vec3 right = getRightDirection();
        glm::vec3 up = getUpDirection();
        
        printf("Transform Debug:\n");
        printf("  Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
        printf("  Rotation: (%.2f°, %.2f°, %.2f°)\n", rot.x, rot.y, rot.z);
        printf("  Scale: (%.2f, %.2f, %.2f)\n", scale.x, scale.y, scale.z);
        printf("  Forward: (%.2f, %.2f, %.2f)\n", forward.x, forward.y, forward.z);
        printf("  Right: (%.2f, %.2f, %.2f)\n", right.x, right.y, right.z);
        printf("  Up: (%.2f, %.2f, %.2f)\n", up.x, up.y, up.z);
        printf("  Up: (%.2f, %.2f, %.2f)\n", up.x, up.y, up.z);
        printf("  Quaternion: (%.2f, %.2f, %.2f, %.2f)\n", 
            m_rotationQuat.x, m_rotationQuat.y, m_rotationQuat.z, m_rotationQuat.w);
    }

  public:
    void serialize(YAML::Emitter &out);
    void deserialize(YAML::detail::iterator_value entity);

  private:
    glm::vec3 m_position{0.0f, 0.0f, 0.0f};           ///< The position of the transform.
    glm::quat m_rotationQuat{1.0f, 0.0f, 0.0f, 0.0f}; ///< The rotation of the transform stored as a quaternion (identity).
    glm::vec3 m_scale{1.0f, 1.0f, 1.0f};              ///< The scale of the transform.

    glm::mat4 m_worldMatrix{1.0f};
    glm::vec3 m_worldPosition{0.0f};
    glm::quat m_worldRotation{1, 0, 0, 0};
    glm::vec3 m_worldScale{1.0f};
};
} // namespace sky