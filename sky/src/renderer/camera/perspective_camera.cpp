#include "perspective_camera.h"

#include "core/events/input.h"
#include "core/application.h"

namespace sky
{
PerspectiveCamera::PerspectiveCamera()
    : m_fov(45.0f), m_aspect(16/9), // Be aware about aspect 1
      m_zNear(0.1f), m_zFar(1000.0f)
{
    recalculatePerpective();

    m_position = glm::vec3(0, 0, -10);
    m_forward = glm::vec3(0, 0, 1); // - Z
    m_up = glm::vec3(0, 1, 0);
}

PerspectiveCamera::PerspectiveCamera(const glm::vec3 &position, float fov, float aspect, float zNear, float zFar)
{
    m_fov = fov;
    m_aspect = aspect;
    m_zNear = zNear;
    m_zFar = zFar;
    m_position = position;
    m_forward = glm::vec3(0, 0, 1);
    m_up = glm::vec3(0, 1, 0);
    
    recalculatePerpective();
}
} // namespace sky