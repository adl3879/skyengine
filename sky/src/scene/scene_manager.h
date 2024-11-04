#pragma once

#include <skypch.h>

#include "scene.h"

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

    void setEditorScene(const Ref<Scene> &scene) { m_editorScene = scene; }
    Ref<Scene> getEditorScene() const { return m_editorScene; }

    SceneState getSceneState() const { return m_sceneState; }
    void setSceneState(SceneState sceneState) { m_sceneState = sceneState; }

  private:
    Ref<Scene> m_activeScene, m_editorScene;
    SceneState m_sceneState;
};
}