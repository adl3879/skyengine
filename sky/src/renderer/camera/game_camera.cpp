#include "game_camera.h"

namespace sky 
{
GameCamera::GameCamera(const glm::vec3& position, const glm::quat& rotation)
    : m_position(position), m_rotation(rotation)
{
}

void GameCamera::setPosition(const glm::vec3& position)
{
    m_position = position;
    m_viewDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setRotation(const glm::quat& rotation)
{
    m_rotation = glm::normalize(rotation);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::setRotation(const glm::vec3& eulerAngles)
{
    glm::vec3 radians = glm::radians(eulerAngles);
    m_rotation = glm::quat(radians);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::translate(const glm::vec3& translation)
{
    m_position += translation;
    m_viewDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::rotate(const glm::quat& rotation)
{
    m_rotation = glm::normalize(m_rotation * rotation);
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::rotate(const glm::vec3& eulerAngles)
{
    glm::vec3 radians = glm::radians(eulerAngles);
    glm::quat rotation = glm::quat(radians);
    rotate(rotation);
}

void GameCamera::lookAt(const glm::vec3& target, const glm::vec3& up)
{
    glm::vec3 forward = glm::normalize(target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    glm::vec3 cameraUp = glm::cross(right, forward);
    
    glm::mat3 rotationMatrix = glm::mat3(right, cameraUp, -forward);
    m_rotation = glm::normalize(glm::quat_cast(rotationMatrix));
    
    m_viewDirty = true;
    m_viewProjectionDirty = true;
    m_vectorsDirty = true;
}

void GameCamera::setProjectionType(ProjectionType type)
{
    if (m_projectionType != type)
    {
        m_projectionType = type;
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setPerspective(float fov, float aspect, float nearPlane, float farPlane)
{
    m_projectionType = ProjectionType::Perspective;
    m_fieldOfView = fov;
    m_aspectRatio = aspect;
    m_nearClipPlane = nearPlane;
    m_farClipPlane = farPlane;
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setOrthographic(float size, float aspect, float nearPlane, float farPlane)
{
    m_projectionType = ProjectionType::Orthographic;
    m_orthographicSize = size;
    m_aspectRatio = aspect;
    m_nearClipPlane = nearPlane;
    m_farClipPlane = farPlane;
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setFieldOfView(float fov)
{
    m_fieldOfView = glm::clamp(fov, 1.0f, 179.0f);
    if (m_projectionType == ProjectionType::Perspective)
    {
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setOrthographicSize(float size)
{
    m_orthographicSize = glm::max(size, 0.01f);
    if (m_projectionType == ProjectionType::Orthographic)
    {
        m_projectionDirty = true;
        m_viewProjectionDirty = true;
    }
}

void GameCamera::setNearClipPlane(float nearPlane)
{
    m_nearClipPlane = glm::max(nearPlane, 0.001f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setFarClipPlane(float farPlane)
{
    m_farClipPlane = glm::max(farPlane, m_nearClipPlane + 0.001f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setAspectRatio(float aspect)
{
    m_aspectRatio = glm::max(aspect, 0.01f);
    m_projectionDirty = true;
    m_viewProjectionDirty = true;
}

void GameCamera::setViewport(float x, float y, float width, float height)
{
    m_viewport = glm::vec4(
        glm::clamp(x, 0.0f, 1.0f),
        glm::clamp(y, 0.0f, 1.0f),
        glm::clamp(width, 0.0f, 1.0f),
        glm::clamp(height, 0.0f, 1.0f)
    );
}

void GameCamera::setDepth(int depth)
{
    m_depth = depth;
}

void GameCamera::setClearFlags(ClearFlags flags)
{
    m_clearFlags = flags;
}

void GameCamera::setBackgroundColor(const glm::vec4& color)
{
    m_backgroundColor = color;
}

glm::vec3 GameCamera::getEulerAngles() const
{
    return glm::degrees(glm::eulerAngles(m_rotation));
}

void GameCamera::updateViewMatrix() const
{
    if (!m_viewDirty) return;
    
    glm::mat4 rotationMatrix = glm::mat4_cast(glm::conjugate(m_rotation));
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -m_position);
    m_viewMatrix = rotationMatrix * translationMatrix;
    
    m_viewDirty = false;
}

void GameCamera::updateProjectionMatrix() const
{
    if (!m_projectionDirty) return;
    
    if (m_projectionType == ProjectionType::Perspective)
    {
        // Use Vulkan-style projection (inverted Y, 0-1 depth)
        m_projectionMatrix = glm::perspective(glm::radians(m_fieldOfView), m_aspectRatio, m_nearClipPlane, m_farClipPlane);
        // Flip Y coordinate for Vulkan
        m_projectionMatrix[1][1] *= -1;
    }
    else
    {
        float width = m_orthographicSize * m_aspectRatio;
        float height = m_orthographicSize;
        // Use Vulkan-style projection (inverted Y, 0-1 depth)
        m_projectionMatrix = glm::ortho(-width, width, height, -height, m_nearClipPlane, m_farClipPlane);
    }
    
    m_projectionDirty = false;
}

void GameCamera::updateViewProjectionMatrix() const
{
    if (!m_viewProjectionDirty) return;
    
    if (m_viewDirty) updateViewMatrix();
    if (m_projectionDirty) updateProjectionMatrix();
    
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
    m_viewProjectionDirty = false;
}

void GameCamera::updateVectors() const
{
    if (!m_vectorsDirty) return;
    
    glm::mat3 rotationMatrix = glm::mat3_cast(m_rotation);
    m_forward = -rotationMatrix[2]; // Negative Z is forward
    m_right = rotationMatrix[0];    // Positive X is right
    m_up = rotationMatrix[1];       // Positive Y is up
    
    m_vectorsDirty = false;
}
}