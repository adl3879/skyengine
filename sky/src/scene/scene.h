#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "renderer/camera/editor_camera.h"
#include "renderer/camera/orthographic_camera.h"
#include "core/uuid.h"
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "renderer/environment.h"
#include "renderer/light_cache.h"

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
    struct ViewportInfo
    {
        glm::vec2 size;
        glm::vec2 mousePos;
        bool isFocus;
    };

  public:
    Scene(const std::string &name = "Untitled", SceneType type = SceneType::Scene3D);
    ~Scene() = default;

    void init();
    void update(float dt);
    void onEvent(Event &e);
    void cleanup();

    Entity createEntity(const std::string &name = std::string());
    Entity createEntityWithUUID(UUID uuid, const std::string &name);
    void destroyEntity(Entity entity);
    void processDestructionQueue(); // Processes the queue

    std::string getName() const { return m_sceneName; }
    void setName(const std::string &name) { m_sceneName = name; }
    SceneType getSceneType() const { return m_sceneType; }
    void setSceneType(SceneType type) { m_sceneType = type; }

    entt::registry &getRegistry() { return m_registry; }
    const auto &getEditorCamera() { return m_editorCamera; }
    const auto &getGameCamera() { return m_orthographicCamera; }
    Entity getEntityFromUUID(UUID uuid);
    LightCache &getLightCache() { return m_lightCache; }
    bool isEditorCameraFreeLook() const { return m_editorCamera->isFreeLook(); }

    LightID addLightToCache(const Light &light, const Transform &transform);
    bool hasDirectionalLight() const { return m_lightCache.getSunlightIndex() > -1; }

    Entity getSelectedEntity();
    void setSelectedEntity(Entity entity);
    void setPath(const fs::path &path) { m_path = path; }
    const fs::path &getPath() const { return m_path; }
    void setViewportInfo(ViewportInfo info) { m_viewportInfo = info; }
    ViewportInfo getViewportInfo() const { return m_viewportInfo; }
    auto getEnvironment() const { return m_environment; }
    void setEnvironment(Environment env) { m_environment = env; } 

    [[nodiscard]] AssetType getType() const override { return AssetType::Scene; }

  private:
    void newScene(const std::string &name);
    void onEntityDestroyed(entt::registry &registry, entt::entity entity);

  private:
    friend class Entity;
    entt::registry m_registry;
    std::string m_sceneName;
    SceneType m_sceneType;
    std::unordered_map<UUID, entt::entity> m_entityMap;
    std::vector<entt::entity> m_destructionQueue;
    ViewportInfo m_viewportInfo;

    entt::entity m_selectedEntity{entt::null};
    fs::path m_path;

    Ref<EditorCamera> m_editorCamera;
    Ref<OrthographicCamera> m_orthographicCamera;

    LightCache m_lightCache;
    Environment m_environment;
};
} // namespace sky