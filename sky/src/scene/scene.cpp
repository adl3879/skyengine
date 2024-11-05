#include "scene.h"

#include "entity.h"
#include "components.h"
#include "scene_manager.h"
#include "core/application.h"

namespace sky
{
Scene::Scene(const std::string &name): m_sceneName(name) 
{
	newScene(name);
    m_registry.on_destroy<entt::entity>().connect<&Scene::onEntityDestroyed>(*this);
}

void Scene::init() {}

void Scene::update(float dt) 
{
    auto sceneState = SceneManager::get().getSceneState();
    if (sceneState == SceneState::Edit)
    {
        auto extent = Application::getWindow()->getExtent();
        m_editorCamera.setViewportSize(extent.width, extent.height);
        m_editorCamera.update(dt);
    }
}

void Scene::cleanup() {}

Entity Scene::createEntity(const std::string &name) 
{
    return createEntityWithUUID(UUID::generate(), name);
}

Entity Scene::createEntityWithUUID(UUID uuid, const std::string& name)
{
    auto entity = Entity{m_registry.create(), this};

    entity.addComponent<IDComponent>() = uuid;
    entity.addComponent<TagComponent>() = name.empty() ? "Unnamed Entity" : name;
    entity.addComponent<TransformComponent>();
    entity.addComponent<HierarchyComponent>();

    m_entityMap[uuid] = entity;

    return entity;
}

void Scene::destroyEntity(Entity entity) 
{
    m_entityMap.erase(entity.getComponent<IDComponent>());
    m_registry.destroy(entity);
}

Entity Scene::getEntityFromUUID(UUID uuid)
{
    return {m_entityMap.at(uuid), this};
}

Camera &Scene::getCamera()
{
    auto sceneState = SceneManager::get().getSceneState();
    if (sceneState == SceneState::Edit) return m_editorCamera;
    else return m_mainCamera;
}

void Scene::newScene(const std::string &name) 
{
}

void Scene::onEntityDestroyed(entt::registry &registry, entt::entity ent)
{
    auto entity = Entity{ ent, this };
    auto &entityHierarchy = entity.getComponent<HierarchyComponent>();
    
    if (entityHierarchy.parent != NULL_UUID)
    {
        auto parentEntity = getEntityFromUUID(entityHierarchy.parent);
        parentEntity.removeChild(entity);
    }

    for (const auto childID : entityHierarchy.children)
    {
        auto childEntity = getEntityFromUUID(childID);
        destroyEntity(childEntity);
    }
}
}