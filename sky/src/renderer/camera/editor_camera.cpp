#include "editor_camera.h"

#include "core/events/input.h"
#include "scene/scene_manager.h"

namespace sky
{
EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_fov(fov), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip)
{
    //m_pitch = 0.2613f;
    updateView();
}

void EditorCamera::updateProjection()
{
    m_aspectRatio = m_viewportWidth / m_viewportHeight;
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearClip, m_farClip);
}

void EditorCamera::updateView()
{
    if (SceneManager::get().sceneIsType(SceneType::Scene2D)) 
        m_yaw = m_pitch = 0.0f; // Lock the camera's rotation

    m_position = calculatePosition();

    glm::quat orientation = getOrientation();
    m_viewMatrix = glm::lookAt(m_position, m_focalPoint, getUpDirection());
}

std::pair<float, float> EditorCamera::panSpeed() const
{
    float x = std::min(m_viewportWidth / 1000.0f, 2.4f); // max = 2.4f
    float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

    float y = std::min(m_viewportHeight / 1000.0f, 2.4f); // max = 2.4f
    float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

    return {xFactor, yFactor};
}

float EditorCamera::rotationSpeed() const { return 0.8f; }

float EditorCamera::zoomSpeed() const
{
    float distance = m_distance * 1.2f;
    distance = std::max(distance, 0.0f);
    float speed = distance * distance;
    speed = std::min(speed, 100.0f); // max speed = 100
    return speed;
}

void EditorCamera::update(float ts)
{
    auto mousePos = Input::getMousePosition();
    const glm::vec2 &mouse{mousePos.x, mousePos.y};
    if (Input::isKeyPressed(Key::LeftAlt))
    {
        glm::vec2 delta = (mouse - m_initialMousePosition) * 0.003f;
        m_initialMousePosition = mouse;

        if (Input::isKeyPressed(Key::LeftControl)) mousePan(delta);
        else if (Input::isMouseButtonPressed(Mouse::ButtonLeft)) mouseRotate(delta);
    }
    m_initialMousePosition = mouse;

    updateView();
}

void EditorCamera::onEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<MouseScrolledEvent>(SKY_BIND_EVENT_FN(EditorCamera::onMouseScrolled));
}

bool EditorCamera::onMouseScrolled(MouseScrolledEvent &e)
{
    float delta = e.getYOffset() * 0.1f;
    mouseZoom(delta);
    updateView();
    return false;
}

void EditorCamera::mousePan(const glm::vec2 &delta)
{
    auto [xSpeed, ySpeed] = panSpeed();
    m_focalPoint += getRightDirection() * delta.x * xSpeed * m_distance;
    m_focalPoint += getUpDirection() * delta.y * ySpeed * m_distance;
}

void EditorCamera::mouseRotate(const glm::vec2 &delta)
{
    float yawSign = getUpDirection().y < 0 ? -1.0f : 1.0f;
    m_yaw += yawSign * delta.x * rotationSpeed();
    m_pitch += delta.y * rotationSpeed();
}

void EditorCamera::mouseZoom(float delta)
{
    m_distance -= delta * zoomSpeed();
    if (m_distance < 1.0f) m_distance = 1.0f;
}

glm::vec3 EditorCamera::getUpDirection() const { return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f)); }

glm::vec3 EditorCamera::getRightDirection() const { return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f)); }

glm::vec3 EditorCamera::getForwardDirection() const
{
    return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 EditorCamera::calculatePosition() const { return m_focalPoint - getForwardDirection() * m_distance; }

glm::quat EditorCamera::getOrientation() const { return glm::quat(glm::vec3(-m_pitch, -m_yaw, 0.0f)); }

void EditorCamera::reset()
{
    m_pitch = 0.2613f;
    m_yaw = 0.0f;
    m_distance = 10.0f;
    m_focalPoint = {0.0f, 0.0f, 0.0f};
    updateView();
}
}