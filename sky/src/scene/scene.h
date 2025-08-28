#pragma once

#include <skypch.h>

#include <entt/entt.hpp>

#include "core/events/event.h"
#include "core/uuid.h"
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "renderer/environment.h"
#include "scene/scene_graph.h"
#include "scene/systems/camera_system.h"
#include "systems/physics_system.h"
#include "systems/transform_system.h"

namespace sky
{
class Entity;

enum class SceneType
{
    Scene2D,
    Scene3D,
    SceneUI
};

std::string sceneTypeToString(SceneType type);
SceneType sceneTypeFromString(const std::string &type);

class Scene : public Asset
{
  public:
    Scene(const std::string &name = "Untitled", SceneType type = SceneType::Scene3D);
    ~Scene() = default;

    void init();
    void update(float dt);
    void fixedUpdate(float dt);
    void onEvent(Event &e);
    void cleanup();

    Entity createEntity(const std::string &name = std::string());
    Entity createEntityWithUUID(UUID uuid, const std::string &name);
    void destroyEntity(Entity entity);

    std::string getName() const { return m_sceneName; }
    void setName(const std::string &name) { m_sceneName = name; }
    SceneType getSceneType() const { return m_sceneType; }
    void setSceneType(SceneType type) { m_sceneType = type; }

    entt::registry &getRegistry() { return m_registry; }
    auto getEntityMap() const { return m_entityMap; }
    void addEntityToMap(UUID uuid, entt::entity entity) { m_entityMap[uuid] = entity; }
    Entity getEntityFromUUID(UUID uuid);
    
    Entity getSelectedEntity();
    void setSelectedEntity(Entity entity);
    const UUID getRootEntityUUID() const { return {0x0000000000000001}; }
    Entity getRootEntity();
    auto getSceneGraph() const { return m_sceneGraph; }
    auto getCameraSystem() { return m_cameraSystem; }
    auto getPhysicsSystem() { return m_physicsSystem; }

    void setPath(const fs::path &path) { m_path = path; }
    const fs::path &getPath() const { return m_path; }

    auto getEnvironment() const { return m_environment; }
    void setEnvironment(Environment env) { m_environment = env; }
    void useEnvironment(); 

    [[nodiscard]] AssetType getType() const override { return AssetType::Scene; }

  public:
    bool sceneViewportIsVisible, gameViewportIsVisible;

  private:
    void newScene(const std::string &name);
    void onEntityDestroyed(entt::registry &registry, entt::entity entity);

  private:
    friend class Entity;
    entt::registry m_registry;
    std::unordered_map<UUID, entt::entity> m_entityMap;
    
    entt::entity m_selectedEntity{entt::null};
    entt::entity m_rootEntity{entt::null}; // Root entity of the scene graph:w
    
    fs::path m_path;

    SceneType m_sceneType;
    
    Ref<SceneGraph> m_sceneGraph;
    std::string m_sceneName;
    Ref<CameraSystem> m_cameraSystem;
    Ref<TransformSystem> m_transformSystem;
    Ref<PhysicsSystem> m_physicsSystem;
    Environment m_environment;
};
} // namespace sky