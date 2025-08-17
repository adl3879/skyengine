#include "camera_system.h"

#include "core/application.h"
#include "scene/components.h"
#include "scene/entity.h"

namespace sky
{
void CameraSystem::setActiveCamera(Entity entity)
{
    if (m_scene->getRegistry().any_of<CameraComponent>(entity))
    {
        m_activeCameraEntity = entity;
    }
}

Entity CameraSystem::getActiveCamera()
{
    return {m_activeCameraEntity, m_scene};
}

GameCamera* CameraSystem::getActiveCameraForRendering()
{
    if (m_activeCameraEntity == entt::null || !m_scene->getRegistry().valid(m_activeCameraEntity))
        return nullptr;
            
    auto entity = getActiveCamera();
    auto *camComp = &entity.getComponent<CameraComponent>();
    if (!camComp || !camComp->isActive) return nullptr;

    return &camComp->camera;
}

void CameraSystem::findAndSetPrimaryCamera()
{
    auto view = m_scene->getRegistry().view<CameraComponent>();
    for (auto entity : view)
    {
        auto& camera = view.get<CameraComponent>(entity);
        if (camera.isPrimary && camera.isActive)
        {
            m_activeCameraEntity = entity;
            return;
        }
    }
    
    // If no primary camera found, use the first active camera
    for (auto entity : view)
    {
        auto& camera = view.get<CameraComponent>(entity);
        if (camera.isActive )
        {
            m_activeCameraEntity = entity;
            return;
        }
    }
}

std::vector<std::pair<Entity, CameraComponent*>> CameraSystem::getSortedCameras() const
{
    std::vector<std::pair<Entity, CameraComponent*>> cameras;
        
    auto view = m_scene->getRegistry().view<CameraComponent>();
    for (auto entity : view)
    {
        auto& camera = view.get<CameraComponent>(entity);
        if (camera.isActive)
        {
            auto e = Entity(entity, m_scene);
            cameras.emplace_back(e, &camera);
        }
    }
    
    // Sort by render order
    std::sort(cameras.begin(), cameras.end(), 
        [](const auto& a, const auto& b) {
            return a.second->renderOrder < b.second->renderOrder;
        });
    
    return cameras;
}

void CameraSystem::update()
{
    auto view = m_scene->getRegistry().view<CameraComponent, TransformComponent>();
    for (auto entity : view)
    {
        auto [camera, transform] = view.get<CameraComponent, TransformComponent>(entity);
        
        // Update camera position and rotation from transform
        camera.camera.setPosition(transform.getPosition());
        camera.camera.setRotation(transform.getRotationQuaternion());

        camera.camera.setAspectRatio(m_scene->getGameViewportInfo().size.x / m_scene->getGameViewportInfo().size.y);

        auto renderer = Application::getRenderer();
        renderer->getDebugLineRenderer().addCameraFrustum(camera.camera);
    }
}
}