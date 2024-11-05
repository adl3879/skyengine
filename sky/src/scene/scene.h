#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "renderer/camera/perspective_camera.h"
#include "renderer/camera/editor_camera.h"
#include "core/uuid.h"

namespace sky
{
class Entity;

class Scene
{
  public:
    Scene(const std::string &name = "Untitled");
    ~Scene() = default;

    void init();
    void update(float dt);
    void cleanup();

    Entity createEntity(const std::string &name = std::string());
    Entity createEntityWithUUID(UUID uuid, const std::string &name);
    void destroyEntity(Entity entity);

    entt::registry &getRegistry() { return m_registry; }
    Camera &getCamera();
    EditorCamera &getEditorCamera() { return m_editorCamera; }
    Entity getEntityFromUUID(UUID uuid);
    //void setMainCamera(Camera &camera) { m_mainCamera = camera; }

  private:
    void newScene(const std::string &name);
    void onEntityDestroyed(entt::registry &registry, entt::entity entity);

  private:
    friend class Entity;
    entt::registry m_registry;
    std::string m_sceneName;
    std::unordered_map<UUID, entt::entity> m_entityMap;

    PerspectiveCamera m_mainCamera;
    EditorCamera m_editorCamera;
};
}