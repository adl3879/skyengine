#pragma once

#include <skypch.h>

#include "scene.h"
#include "core/filesystem.h"

namespace sky
{
class SceneManager
{
  public:
    static SceneManager &get();

    SceneManager();
    ~SceneManager() = default;

    void setActiveScene(const Ref<Scene> &scene) { m_activeScene = scene; }
    Ref<Scene> getActiveScene() const { return m_activeScene; }
    Ref<Scene> getEditorScene() const { return m_editorScene; }

    void reset();
    bool sceneIsType(SceneType type) const;
    void openScene(const fs::path &path);
    void closeScene(const fs::path &path);
    void saveActiveScene();
    void saveAll();

    Ref<Scene> cloneScene(const Ref<Scene> &scene);

  public:
    void enterPlayMode();
    void exitPlayMode();

  private:
    Ref<Scene> m_activeScene;
    Ref<Scene> m_editorScene;
    bool m_inPlayMode = false;
};
} // namespace sky