#include "scene_manager.h"

#include "core/log/log.h"
#include "core/resource/custom_thumbnail.h"
#include "entity.h"
#include "physics/physics_manager.h"
#include "scene/components.h"
#include "scene_serializer.h"
#include "asset_management/asset_manager.h"
#include "component_list.h"
#include "systems/transform_system.h"
#include <glm/fwd.hpp>

namespace sky
{
// Helper: apply a function to each type in tuple
template<typename Tuple, typename Func, std::size_t... I>
void forEachTypeImpl(Func &&f, std::index_sequence<I...>) {
    (f(std::type_identity<std::tuple_element_t<I, Tuple>>{}), ...);
}

template<typename Tuple, typename Func>
void forEachType(Func &&f) {
    forEachTypeImpl<Tuple>(std::forward<Func>(f), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

void cloneAllComponents(entt::registry &srcRegistry,
    entt::registry &dstRegistry,
    const std::unordered_map<entt::entity, entt::entity> &entityMap) {
    forEachType<ComponentList>([&](auto typeTag) {
        using Component = typename decltype(typeTag)::type;

        auto view = srcRegistry.view<Component>();
        for (auto srcEntity : view) {
            auto dstEntity = entityMap.at(srcEntity);
            dstRegistry.emplace_or_replace<Component>(dstEntity, view.get<Component>(srcEntity));
        }
    });
} 

// Main cloning function
template<typename ComponentTuple, typename... ExcludedComponents>
void cloneAllComponentsExcluding(entt::registry &srcRegistry,
    entt::registry &dstRegistry,
    const std::unordered_map<entt::entity, entt::entity> &entityMap) {
    forEachType<ComponentTuple>([&](auto typeTag) {
        using Component = typename decltype(typeTag)::type;

        // Skip excluded types
        if constexpr (!(std::disjunction_v<std::is_same<Component, ExcludedComponents>...>)) {
            auto view = srcRegistry.view<Component>();
            for (auto srcEntity : view) {
                auto dstEntity = entityMap.at(srcEntity);
                dstRegistry.emplace_or_replace<Component>(dstEntity, view.get<Component>(srcEntity));
            }
        }
    });
}

SceneManager &SceneManager::get()
{
    static SceneManager instance;
    return instance;
}

void SceneManager::init()
{
    m_editorCamera = CreateRef<EditorCamera>(45.f, 16 / 9, 0.1f, 1000.f);
    m_editorScene = CreateRef<Scene>();
    m_gameScene = CreateRef<Scene>("Game Scene");
    // initialize physics manager with the editor scene
    physics::PhysicsManager::get().init();
    m_editorScene->getPhysicsSystem()->init();
}

void SceneManager::reset() 
{
    m_editorScene = CreateRef<Scene>();
}

void SceneManager::openScene(const fs::path &path) 
{
    auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Scene);
    AssetManager::getAssetAsync<Scene>(handle, [=, this](const Ref<Scene> &scene) 
    {
        m_editorScene = scene;
        m_editorScene->setPath(path);
        m_editorScene->useEnvironment();

        m_gameScene = m_editorScene;
    });
}

void SceneManager::closeScene(const fs::path &path) 
{
    m_editorScene = CreateRef<Scene>();
}

void SceneManager::saveActiveScene() 
{
    SceneSerializer serializer(m_editorScene);
    auto path = ProjectManager::getConfig().getAssetDirectory() / m_editorScene->getPath();
    serializer.serialize(path, m_editorScene->handle);

    AssetManager::serializeAssetDirectory();
    SKY_CORE_INFO("{} saved", path.string());

    CustomThumbnail::get().refreshSceneThumbnail(m_editorScene->getPath());
}

void SceneManager::saveAll() 
{
}

bool SceneManager::sceneIsType(SceneType type) const 
{
    if (m_editorScene == nullptr) return false;
    
    return m_editorScene->getSceneType() == type;
}

Ref<Scene> SceneManager::cloneScene(const Ref<Scene> &source)
{
    auto newScene = CreateRef<Scene>(source->getName() + " (Copy)", source->getSceneType());
    newScene->setPath(source->getPath());
    newScene->setEnvironment(source->getEnvironment());

    auto &srcRegistry = source->getRegistry();
    auto &dstRegistry = newScene->getRegistry();

    std::unordered_map<entt::entity, entt::entity> entityMap;

    auto idView = srcRegistry.view<IDComponent>();
    for (const auto entity : idView)
    {
        auto dstEntity = dstRegistry.create();
        entityMap[entity] = dstEntity;
        newScene->addEntityToMap(srcRegistry.get<IDComponent>(entity), dstEntity);
    }
    
    // Clone all components except these
    cloneAllComponents(srcRegistry, dstRegistry, entityMap);

    return newScene;
}

void SceneManager::enterPlayMode()
{
    if (isInPlayMode()) return;

    m_gameScene = cloneScene(m_editorScene);

    // physics manager should now use the game scene
    m_gameScene->getPhysicsSystem()->init();
    m_gameScene->getCameraSystem()->findAndSetPrimaryCamera(); 
    
    physics::PhysicsManager::get().setScene(m_gameScene.get());
    physics::PhysicsManager::get().start();

    m_sceneState = SceneState::Play;
}

void SceneManager::exitPlayMode()
{
    if (!isInPlayMode()) return;

    physics::PhysicsManager::get().stop();
    physics::PhysicsManager::get().reset();
    
    m_gameScene.reset();
    m_gameScene = m_editorScene;

    m_sceneState = SceneState::Edit;
}

void SceneManager::update(float dt)
{
    m_editorCamera->update(dt);

    m_gameScene->update(dt);
    m_editorScene->update(dt);
}

void SceneManager::fixedUpdate(float dt)
{
    m_gameScene->fixedUpdate(dt);
    m_editorScene->fixedUpdate(dt);
}

void SceneManager::onEvent(Event &e)
{
    m_editorCamera->onEvent(e);

    m_gameScene->onEvent(e);
    m_editorScene->onEvent(e);
}

void SceneManager::cleanup()
{
    m_gameScene->cleanup();
    m_editorScene->cleanup();
}
} // namespace sky