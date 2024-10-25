#include "camera.h"

#include "core/events/input.h"
#include "core/application.h"

namespace sky
{
Camera::Camera()
    : m_fov(45.0f), m_aspect(1), // Be aware about aspect 1
      m_zNear(0.1f), m_zFar(1000.0f)
{
    recalculatePerpective();

    m_position = glm::vec3(0, 0, -10);
    m_forward = glm::vec3(0, 0, 1); // - Z
    m_up = glm::vec3(0, 1, 0);
}

Camera::Camera(const glm::vec3 &position, float fov, float aspect, float zNear, float zFar)
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

void Camera::update(float dt)
{
    if (Input::isKeyPressed(Key::Up)) moveForward(dt);
    if (Input::isKeyPressed(Key::Down)) moveBackward(dt);
    if (Input::isKeyPressed(Key::Left)) moveLeft(dt);
    if (Input::isKeyPressed(Key::Right)) moveRight(dt);

    // mouse
    if (Input::isMouseButtonPressed(Mouse::ButtonRight))
	{
        const auto mouseDelta = Input::getMouseDelta();
		rotate(mouseDelta.x, mouseDelta.y, 1.f);
	}
}
} // namespace sky