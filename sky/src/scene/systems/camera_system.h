#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "renderer/camera/game_camera.h"
#include "scene/components.h"

namespace sky
{
class Entity;
class Scene;

class CameraSystem
{
  public:
    CameraSystem(Scene* scene) : m_scene(scene) {}

    void setActiveCamera(Entity entity);
    Entity getActiveCamera();
    GameCamera* getActiveCameraForRendering();
    void findAndSetPrimaryCamera();
    std::vector<std::pair<Entity, CameraComponent*>> getSortedCameras() const;
    void update();

  private:
    Scene* m_scene = nullptr;
    entt::entity m_activeCameraEntity = entt::null;
};
}