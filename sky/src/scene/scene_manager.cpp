#include "scene_manager.h"

#include "scene_serializer.h"
#include "asset_management/asset_manager.h"

namespace sky
{
SceneManager &SceneManager::get()
{
    static SceneManager instance;
    return instance;
}

SceneManager::SceneManager()
{
    m_sceneState = SceneState::Edit;
    m_activeScene = CreateRef<Scene>();
}

void SceneManager::openScene(const fs::path &path) 
{
    m_currentViewport = path;
    m_openedScenes.insert(path);
    auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Scene);
    AssetManager::getAssetAsync<Scene>(handle, [=](const Ref<Scene> &scene) 
    {
        m_activeScene = scene;
		m_activeScene->init();
        m_activeScene->setPath(path);
    });
}

void SceneManager::closeScene(const fs::path &path) 
{
    m_openedScenes.erase(path);
    if (!m_openedScenes.empty()) openScene(*m_openedScenes.rbegin());
    else 
    {
        m_activeScene = CreateRef<Scene>();
        m_activeScene->init();
    }
}

std::set<fs::path> SceneManager::getOpenedScenes() 
{
    if (m_openedScenes.size() == 0) return std::set<fs::path>{"[empty]"};
    else return m_openedScenes;
}

void SceneManager::saveActiveScene() 
{
    SceneSerializer serializer(m_activeScene);
    auto path = ProjectManager::getConfig().getAssetDirectory() / m_activeScene->getPath();
    serializer.serialize(path, m_activeScene->handle);

    AssetManager::serializeAssetDirectory();
    SKY_CORE_INFO("{} saved", path.string());
}

void SceneManager::saveAll() 
{
    for (const auto &scene : m_openedScenes)
    {
		auto handle = AssetManager::getOrCreateAssetHandle(scene, AssetType::Scene);
		SceneSerializer serializer(AssetManager::getAsset<Scene>(handle));
		auto path = ProjectManager::getConfig().getAssetDirectory() / scene;
		serializer.serialize(path);
    }
}
} // namespace sky