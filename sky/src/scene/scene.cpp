#include "scene.h"

#include "entity.h"
#include "components.h"
#include "renderer/camera/editor_camera.h"
#include "scene_manager.h"
#include "core/application.h"
#include "skypch.h"

namespace sky
{
Scene::Scene(const std::string &name, SceneType type)
    : m_sceneName(name), m_sceneType(type) 
{
	newScene(name);
    m_registry.on_destroy<HierarchyComponent>().connect<&Scene::onEntityDestroyed>(*this);
}

void Scene::init() 
{
    m_lightCache.init(Application::getRenderer()->getDevice());
    
    m_editorCamera = CreateRef<EditorCamera>(45.f, 16 / 9, 0.1f, 1000.f);

    m_orthographicCamera = CreateRef<OrthographicCamera>(-1.0f, 1.0f, -1.0f, 1.0f);
    m_orthographicCamera->setProjection(16/9, 1.f);
}

void Scene::update(float dt) 
{
    auto sceneState = SceneManager::get().getSceneState();
    if (sceneState == SceneState::Edit)
    {
        m_editorCamera->setViewportSize(m_viewportInfo.size);
        m_editorCamera->update(dt);
    }
}

void Scene::onEvent(Event& e)
{
    if (m_viewportInfo.isFocus) m_editorCamera->onEvent(e);
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
    entity.addComponent<VisibilityComponent>() = true;

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

    m_selectedEntity = Entity{entt::null, this};
    m_destructionQueue.push_back(entity);
}

void Scene::processDestructionQueue()
{
    for (auto entity : m_destructionQueue)
    {
        if (m_registry.valid(entity))
        {
            m_registry.destroy(entity); // Safely destroy the entity
        }
    }
    m_destructionQueue.clear(); // Clear the queue after destruction
}

Entity Scene::getEntityFromUUID(UUID uuid)
{
    return {m_entityMap.at(uuid), this};
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

Entity Scene::getSelectedEntity() 
{
    return Entity{m_selectedEntity, this};
}

void Scene::setSelectedEntity(Entity entity) 
{
    if (m_registry.valid(entity)) m_selectedEntity = entity.getEntityID();
}

LightID Scene::addLightToCache(const Light &light, const Transform &transform)
{
    return m_lightCache.addLight(light, transform);
}

std::string sceneTypeToString(SceneType type)
{
    switch (type)
    {
        case SceneType::Scene2D: return "Scene2D";
        case SceneType::Scene3D: return "Scene3D";
        case SceneType::SceneUI: return "SceneUI";
        default: return "Scene3D"; // Default to 3D if typ
    }
}

SceneType sceneTypeFromString(const std::string &type)
{
    if (type == "Scene2D") return SceneType::Scene2D;
    if (type == "Scene3D") return SceneType::Scene3D;
    if (type == "SceneUI") return SceneType::SceneUI;
    return SceneType::Scene3D;
}
}