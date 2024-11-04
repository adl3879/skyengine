#pragma once

#include "camera.h"
#include "core/events/event.h"
#include "core/events/mouse_event.h"

namespace sky
{
class EditorCamera : public Camera
{
  public:
    EditorCamera() = default;
    EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

    SKY_INLINE glm::mat4 getViewProjection() const override { return m_projectionMatrix * m_viewMatrix; }
    SKY_INLINE RenderData getRenderData() const override 
    {
        return {
            getViewProjection(), 
            m_viewMatrix, 
            m_projectionMatrix, 
            m_position,
            getForwardDirection(),
            m_nearClip, 
            m_farClip};
    }

    void update(float ts);
    void onEvent(Event &e);

    SKY_INLINE float getDistance() const { return m_distance; }
    SKY_INLINE void setDistance(float distance) { m_distance = distance; }

    SKY_INLINE void setViewportSize(float width, float height)
    {
        m_viewportWidth = width;
        m_viewportHeight = height;
        updateProjection();
    }

    const glm::mat4 &getViewMatrix() const { return m_viewMatrix; }
    const glm::mat4 &getProjectionMatrix() const { return m_projectionMatrix; }

    glm::vec3 getUpDirection() const;
    glm::vec3 getRightDirection() const;
    glm::vec3 getForwardDirection() const;
    glm::quat getOrientation() const;

    float getPitch() const { return m_pitch; }
    float getYaw() const { return m_yaw; }

    bool onMouseScrolled(MouseScrolledEvent &e);

    void reset();

  private:
    void updateProjection();
    void updateView();

    void mousePan(const glm::vec2 &delta);
    void mouseRotate(const glm::vec2 &delta);
    void mouseZoom(float delta);

    glm::vec3 calculatePosition() const;

    std::pair<float, float> panSpeed() const;
    float rotationSpeed() const;
    float zoomSpeed() const;

  private:
    float m_fov = 45.0f, m_aspectRatio = 1.778f, m_nearClip = 0.1f, m_farClip = 1000.0f;

    glm::mat4 m_viewMatrix, m_projectionMatrix;
    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_focalPoint = {0.0f, 0.0f, 0.0f};

    glm::vec2 m_initialMousePosition = {0.0f, 0.0f};

    float m_distance = 10.0f;
    float m_pitch = 0.0f, m_yaw = 0.0f;

    float m_viewportWidth = 1280, m_viewportHeight = 720;
    float m_deltaTime = 0.0f;
};
} // namespace sky