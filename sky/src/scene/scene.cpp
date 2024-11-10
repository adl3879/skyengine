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
    m_registry.on_destroy<HierarchyComponent>().connect<&Scene::onEntityDestroyed>(*this);
}

void Scene::init() {}

void Scene::update(float dt) 
{
    auto sceneState = SceneManager::get().getSceneState();
    if (sceneState == SceneState::Edit)
    {
        m_editorCamera.setViewportSize(m_viewportInfo.size);
        m_editorCamera.update(dt);
    }
}

void Scene::onEvent(Event& e)
{
    if (m_viewportInfo.isFocus) m_editorCamera.onEvent(e);
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
    auto &hierarchy = entity.getComponent<HierarchyComponent>();
    if (hierarchy.parent != NULL_UUID)
    {
        // remove child from parent's children list
        auto parentEntity = getEntityFromUUID(hierarchy.parent);
        parentEntity.removeChild(entity);
    }

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
    auto entity = Entity{ent, this};
    auto &entityHierarchy = entity.getComponent<HierarchyComponent>();

    for (const auto childID : entityHierarchy.children)
    {
        auto childEntity = getEntityFromUUID(childID);
        destroyEntity(childEntity);
    }
}
}