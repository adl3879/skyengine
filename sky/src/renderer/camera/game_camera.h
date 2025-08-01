#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sky 
{
enum class ProjectionType
{
    Perspective,
    Orthographic
};

enum class ClearFlags
{
    Skybox,
    SolidColor,
    DepthOnly,
    DontClear
};

class GameCamera : public Camera
{
  public:
    GameCamera() = default;
    GameCamera(const glm::vec3& position, const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    SKY_INLINE const glm::mat4& getView() const override 
    { 
        if (m_viewDirty) updateViewMatrix();
        return m_viewMatrix; 
    }
    
    SKY_INLINE const glm::mat4& getProjection() const override 
    { 
        if (m_projectionDirty) updateProjectionMatrix();
        return m_projectionMatrix; 
    }
    
    SKY_INLINE const glm::mat4& getViewProjection() override 
    { 
        if (m_viewProjectionDirty) updateViewProjectionMatrix();
        return m_viewProjectionMatrix; 
    }
    
    SKY_INLINE const glm::vec3& getPosition() const override { return m_position; }
    SKY_INLINE const glm::vec3& getCameraDir() const override 
    { 
        if (m_vectorsDirty) updateVectors();
        return m_forward; 
    }
    SKY_INLINE const float& getNear() const override { return m_nearClipPlane; }
    SKY_INLINE const float& getFar() const override { return m_farClipPlane; }
    SKY_INLINE const float& getAspect() const override { return m_aspectRatio; }

    // Transform methods
    void setPosition(const glm::vec3& position);
    void setRotation(const glm::quat& rotation);
    void setRotation(const glm::vec3& eulerAngles); // In degrees
    
    void translate(const glm::vec3& translation);
    void rotate(const glm::quat& rotation);
    void rotate(const glm::vec3& eulerAngles); // In degrees
    
    void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Projection methods
    void setProjectionType(ProjectionType type);
    void setPerspective(float fov, float aspect, float nearPlane, float farPlane);
    void setOrthographic(float size, float aspect, float nearPlane, float farPlane);
    
    // Property setters
    void setFieldOfView(float fov);
    void setOrthographicSize(float size);
    void setNearClipPlane(float nearPlane);
    void setFarClipPlane(float farPlane);
    void setAspectRatio(float aspect);

    // Viewport and rendering
    void setViewport(float x, float y, float width, float height);
    void setDepth(int depth);
    void setClearFlags(ClearFlags flags);
    void setBackgroundColor(const glm::vec4& color);

    // Getters
    const glm::quat& getRotation() const { return m_rotation; }
    glm::vec3 getEulerAngles() const; // Returns in degrees
    
    const glm::vec3& getForward() const 
    { 
        if (m_vectorsDirty) updateVectors();
        return m_forward; 
    }
    
    const glm::vec3& getRight() const 
    { 
        if (m_vectorsDirty) updateVectors();
        return m_right; 
    }
    
    const glm::vec3& getUp() const 
    { 
        if (m_vectorsDirty) updateVectors();
        return m_up; 
    }
    
    ProjectionType getProjectionType() const { return m_projectionType; }
    float getFieldOfView() const { return m_fieldOfView; }
    float getOrthographicSize() const { return m_orthographicSize; }
    
    const glm::vec4& getViewport() const { return m_viewport; }
    int getDepth() const { return m_depth; }
    ClearFlags getClearFlags() const { return m_clearFlags; }
    const glm::vec4& getBackgroundColor() const { return m_backgroundColor; }

  private:
    // Transform
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    // Projection settings
    ProjectionType m_projectionType = ProjectionType::Perspective;
    
    // Perspective settings
    float m_fieldOfView = 60.0f;  // In degrees
    
    // Orthographic settings
    float m_orthographicSize = 5.0f;  // Half-height of the view volume
    
    // Common projection settings
    float m_nearClipPlane = 0.1f;
    float m_farClipPlane = 1000.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    
    // Viewport settings
    glm::vec4 m_viewport = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // x, y, width, height (normalized 0-1)
    int m_depth = 0; // Rendering order
    
    // Rendering settings
    ClearFlags m_clearFlags = ClearFlags::Skybox;
    glm::vec4 m_backgroundColor = glm::vec4(0.19f, 0.3f, 0.47f, 1.0f); // Unity default blue
    
    // Cached matrices
    mutable glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    mutable glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
    mutable glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);
    
    // Cached vectors
    mutable glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, -1.0f);
    mutable glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    mutable glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Dirty flags
    mutable bool m_viewDirty = true;
    mutable bool m_projectionDirty = true;
    mutable bool m_viewProjectionDirty = true;
    mutable bool m_vectorsDirty = true;

    void updateViewMatrix() const;
    void updateProjectionMatrix() const;
    void updateViewProjectionMatrix() const;
    void updateVectors() const;
};
}