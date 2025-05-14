#include "editor_camera.h"

#include "core/events/input.h"
#include "core/events/key_codes.h"
#include "core/events/key_event.h"
#include "core/log/log.h"
#include "scene/scene_manager.h"
#include "skypch.h"

namespace sky
{
EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_fov(fov), m_aspectRatio(aspectRatio), m_nearClip(nearClip), m_farClip(farClip)
{
    if (SceneManager::get().sceneIsType(SceneType::Scene3D)) 
        m_pitch = 0.2613f;

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

    
    glm::quat orientation = getOrientation();
    if (!m_isFreeLook)
    {
        m_position = calculatePosition();
        m_viewMatrix = glm::lookAt(m_position, m_focalPoint, getUpDirection());
    }
    else 
    {
        glm::vec3 forward = glm::rotate(orientation, glm::vec3(0.0f, 0.0f, -1.0f));
        m_viewMatrix = glm::lookAt(m_position, m_position + forward, getUpDirection());
    }
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
    if (m_isFreeLook)
        updateFreeLook(ts);
    else
        updateOrbit(ts);

    updateView();
}

void EditorCamera::onEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<MouseScrolledEvent>(SKY_BIND_EVENT_FN(EditorCamera::onMouseScrolled));
    dispatcher.dispatch<KeyPressedEvent>(SKY_BIND_EVENT_FN(EditorCamera::onKeyPressed));
}

bool EditorCamera::onMouseScrolled(MouseScrolledEvent &e)
{
    float delta = e.getYOffset() * 0.1f;
    mouseZoom(delta);
    updateView();
    return false;
}

bool EditorCamera::onKeyPressed(KeyPressedEvent &e)
{
    if (Input::isKeyPressed(Key::LeftShift))
    {
        if (e.getKeyCode() == Key::F)
        {
            m_isFreeLook = !m_isFreeLook;
            Input::showMouseCursor(!m_isFreeLook);
        }
    }
    return true;
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
    if (m_distance > 200.0f) m_distance = 200.0f; // Add maximum zoom distance limit
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

void EditorCamera::updateFreeLook(float dt)
{
    glm::vec3 direction{0.0f};

    if (Input::isKeyPressed(Key::W)) direction += getForwardDirection();
    if (Input::isKeyPressed(Key::S)) direction -= getForwardDirection();
    if (Input::isKeyPressed(Key::A)) direction -= getRightDirection();
    if (Input::isKeyPressed(Key::D)) direction += getRightDirection();
    if (Input::isKeyPressed(Key::Q)) direction -= getUpDirection();
    if (Input::isKeyPressed(Key::E)) direction += getUpDirection();

    if (glm::length(direction) > 0.0f)
        m_position += glm::normalize(direction) * 5.f * dt;

    // Mouse look
    glm::vec2 mousePos = Input::getMousePosition();
    glm::vec2 delta = (mousePos - m_initialMousePosition) * 0.002f;
    m_initialMousePosition = mousePos;

    m_yaw   -= delta.x;
    m_pitch -= delta.y;
    m_pitch = std::clamp(m_pitch, -1.5f, 1.5f);
}

void EditorCamera::updateOrbit(float dt)
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
}
}