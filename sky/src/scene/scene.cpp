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
    auto entity = Entity{m_registry.create(), this};

    entity.addComponent<TagComponent>() = name;
    entity.addComponent<TransformComponent>();

    return entity;
}

void Scene::destroyEntity(Entity entity) 
{
    m_registry.destroy(entity); }

Camera &Scene::getCamera()
{
    auto sceneState = SceneManager::get().getSceneState();
    if (sceneState == SceneState::Edit) return m_editorCamera;
    else return m_mainCamera;
}

void Scene::newScene(const std::string &name) 
{
}
}