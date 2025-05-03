#pragma once

#include <skypch.h>
#include <glm/glm.hpp>
#include "camera.h"

namespace sky
{
class OrthographicCamera : public Camera
{
  public:
    OrthographicCamera(float left, float right, float bottom, float top);

    void setProjection(float left, float right, float bottom, float top);
    void setProjection(float aspectRatio, float zoom);
    void setPosition(const glm::vec3& position) { m_position = position; recalculateViewMatrix(); }
    void setRotation(float rotation) { m_rotation = rotation; recalculateViewMatrix(); }

    float getRotation() const { return m_rotation; }
    
    const glm::vec3& getPosition() const override { return m_position; }
    const glm::mat4& getProjection() const override { return m_projectionMatrix; }
    const glm::mat4& getView() const override { return m_viewMatrix; }
    const glm::mat4& getViewProjection() override { return m_viewProjectionMatrix; }
    const glm::vec3 &getCameraDir() const override { return glm::vec3{}; }
    const float &getNear() const override { return 0; }
    const float &getFar() const override { return 0; }
    const float &getAspect() const override { return 0; }

    const float &getAspectRatio() const { return m_aspectRatio; }
    const float &getZoom() const { return m_zoom; }

  private:
    void recalculateViewMatrix();

  private:
    glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewProjectionMatrix = glm::mat4(1.0f);

    glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
    float m_rotation = 0.0f;
    float m_zoom = 1.f;
    float m_aspectRatio = 1.f;
};
}