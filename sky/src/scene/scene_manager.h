#pragma once

#include <skypch.h>

#include "scene.h"
#include "core/filesystem.h"

namespace sky
{
enum class SceneState
{
    Edit,
    Play
};

class SceneManager
{
  public:
    static SceneManager &get();

    SceneManager();
    ~SceneManager() = default;

    void reset();

    void setActiveScene(const Ref<Scene> &scene) { m_activeScene = scene; }
    Ref<Scene> getActiveScene() const { return m_activeScene; }
    bool sceneIsType(SceneType type) const;

    SceneState getSceneState() const { return m_sceneState; }
    void setSceneState(SceneState sceneState) { m_sceneState = sceneState; }

    void openScene(const fs::path &path);
    void closeScene(const fs::path &path);
    void saveActiveScene();
    void saveAll();

  private:
    Ref<Scene> m_activeScene = nullptr;
    SceneState m_sceneState;
};
} // namespace sky