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
 
    SKY_INLINE const glm::mat4 &getView() const override { return m_viewMatrix; }
    SKY_INLINE const glm::mat4 &getProjection() const override { return m_projectionMatrix; }
    SKY_INLINE const glm::vec4 &getPosition() const override { return glm::vec4{m_position, 1.f}; };
    SKY_INLINE const glm::mat4 &getViewProjection() override { return m_projectionMatrix * m_viewMatrix; }

    void update(float ts);
    void onEvent(Event &e);

    SKY_INLINE float getDistance() const { return m_distance; }
    SKY_INLINE void setDistance(float distance) { m_distance = distance; }

    SKY_INLINE void setViewportSize(glm::vec2 size)
    {
        m_viewportWidth = size.x;
        m_viewportHeight = size.y;
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
    float m_fov, m_aspectRatio, m_nearClip, m_farClip;

    glm::mat4 m_viewMatrix, m_projectionMatrix;
    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    glm::vec3 m_focalPoint = {0.0f, 0.0f, 0.0f};

    glm::vec2 m_initialMousePosition = {0.0f, 0.0f};

    float m_distance = 10.0f;
    float m_pitch = 0.0f, m_yaw = 0.0f;
    float m_viewportWidth = 1280, m_viewportHeight = 720;
};
} // namespace sky