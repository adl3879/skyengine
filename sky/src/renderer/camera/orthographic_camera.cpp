#include "orthographic_camera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace sky
{
OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
{
    setProjection(left, right, bottom, top);
}

void OrthographicCamera::setProjection(float left, float right, float bottom, float top)
{
    m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

void OrthographicCamera::setProjection(float aspectRatio, float zoom) 
{
    m_aspectRatio = aspectRatio;
    m_zoom = zoom;
    setProjection(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom);
}

void OrthographicCamera::recalculateViewMatrix()
{
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position) *
        glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0, 0, 1));

    m_viewMatrix = glm::inverse(transform);
    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}
}