#pragma once

#include <skypch.h>

#include "scene.h"
#include "core/filesystem.h"

namespace sky
{
enum class SceneState
{
    Edit, Play
};

class SceneManager
{
  public:
    static SceneManager &get();

    SceneManager();
    ~SceneManager() = default;

    void setActiveScene(const Ref<Scene> &scene) { m_activeScene = scene; }
    Ref<Scene> getActiveScene() const { return m_activeScene; }

    SceneState getSceneState() const { return m_sceneState; }
    void setSceneState(SceneState sceneState) { m_sceneState = sceneState; }

    void openScene(const fs::path &path);
    void closeScene(const fs::path &path);
    void saveActiveScene();
    void saveAll();
    std::set<fs::path> getOpenedScenes();

  private:
    Ref<Scene> m_activeScene;
    SceneState m_sceneState;
    std::set<fs::path> m_openedScenes;
};
}