#include "scene.h"

#include "entity.h"
#include "components.h"
#include "physics/physics_manager.h"
#include "renderer/camera/editor_camera.h"
#include "scene/components.h"
#include "scene_manager.h"
#include "core/application.h"
#include "asset_management/asset_manager.h"
#include "skypch.h"
#include "renderer/texture.h"
#include "core/helpers/image.h"

namespace sky
{
Scene::Scene(const std::string &name, SceneType type)
    : m_sceneName(name), m_sceneType(type) 
{
    init();
}

void Scene::init() 
{
    m_sceneGraph = CreateRef<SceneGraph>(this);
    m_cameraSystem = CreateRef<CameraSystem>(this);
    m_transformSystem = CreateRef<TransformSystem>(this);
    m_physicsSystem = CreateRef<PhysicsSystem>(this);

	newScene(m_sceneName);

    m_rootEntity = createEntityWithUUID(getRootEntityUUID(), "Root");
    m_physicsSystem->init();
}

void Scene::update(float dt)
{
    m_cameraSystem->update();
    m_transformSystem->update();
    m_physicsSystem->draw();
}

void Scene::fixedUpdate(float dt)
{
    m_physicsSystem->fixedUpdate(dt);
}

void Scene::onEvent(Event& e)
{
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
    entity.addComponent<RelationshipComponent>();
    entity.addComponent<VisibilityComponent>() = true;

    m_entityMap[uuid] = entity;

    return entity;
}

void Scene::destroyEntity(Entity entity)
{
    if (!m_registry.valid(entity)) return;

    m_entityMap.erase(entity.getComponent<IDComponent>());
    m_registry.destroy(entity);
    setSelectedEntity({m_rootEntity, this});
}

Entity Scene::getEntityFromUUID(UUID uuid)
{
    auto it = m_entityMap.find(uuid);
    if (it != m_entityMap.end())
        return {it->second, this};

    return {}; // return invalid Entity
}

void Scene::newScene(const std::string &name) 
{
}

void Scene::onEntityDestroyed(entt::registry &registry, entt::entity ent)
{
}

Entity Scene::getSelectedEntity() 
{
    return Entity{m_selectedEntity, this};
}

Entity Scene::getRootEntity()
{
    return Entity{m_rootEntity, this};
}

void Scene::setSelectedEntity(Entity entity) 
{
    if (m_registry.valid(entity)) m_selectedEntity = entity.getEntityID();
}

void Scene::useEnvironment()
{
    auto env = m_environment;
    auto renderer = Application::getRenderer();
    if (env.skyboxHandle != NULL_UUID)
    {
        AssetManager::getAssetAsync<TextureCube>(env.skyboxHandle, [=](const Ref<TextureCube> &hdrTex){
            auto hdrImageId = helper::loadImageFromTexture(hdrTex, VK_FORMAT_R32G32B32A32_SFLOAT);
            renderer->getIBL().setHdrImageId(hdrImageId);
        });
    }
    else 
    {
        renderer->getIBL().setHdrImageId(NULL_IMAGE_ID);
    }
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