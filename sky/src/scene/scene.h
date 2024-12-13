#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "renderer/camera/perspective_camera.h"
#include "renderer/camera/editor_camera.h"
#include "core/uuid.h"
#include "core/filesystem.h"
#include "asset_management/asset.h"
#include "renderer/light_cache.h"

namespace sky
{
class Entity;

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
    Scene(const std::string &name = "Untitled");
    ~Scene() = default;

    void init();
    void update(float dt);
    void onEvent(Event &e);
    void cleanup();

    Entity createEntity(const std::string &name = std::string());
    Entity createEntityWithUUID(UUID uuid, const std::string &name);
    void destroyEntity(Entity entity);

    entt::registry &getRegistry() { return m_registry; }
    Camera &getCamera();
    EditorCamera &getEditorCamera() { return m_editorCamera; }
    Entity getEntityFromUUID(UUID uuid);
    LightCache &getLightCache() { return m_lightCache; }

	LightID addLightToCache(const Light &light, const Transform &transform); 
    bool hasDirectionalLight() const { return m_lightCache.getSunlightIndex() > -1; }

	Entity getSelectedEntity();
    void setSelectedEntity(Entity entity);

    void setPath(const fs::path &path) { m_path = path; }
    const fs::path &getPath() const { return m_path; }

    void setViewportInfo(ViewportInfo info) { m_viewportInfo = info; }
    ViewportInfo getViewportInfo() const { return m_viewportInfo; }

	[[nodiscard]] AssetType getType() const override { return AssetType::Scene; }

  private:
    void newScene(const std::string &name);
    void onEntityDestroyed(entt::registry &registry, entt::entity entity);

  private:
    friend class Entity;
    entt::registry m_registry;
    std::string m_sceneName;
    std::unordered_map<UUID, entt::entity> m_entityMap;
    ViewportInfo m_viewportInfo;

    entt::entity m_selectedEntity{entt::null};
    fs::path m_path;

    PerspectiveCamera m_mainCamera;
    EditorCamera m_editorCamera{45.f, 16/9, 0.1f, 1000.f};

    LightCache m_lightCache;
};
}