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
 
    void init();
    void update(float dt);
    void fixedUpdate(float dt);
    void onEvent(Event &e);
    void cleanup();
    
    void setGameScene(const Ref<Scene> &scene) { m_gameScene = scene; }
    Ref<Scene> getGameScene() const { return m_gameScene; }
    Ref<Scene> getEditorScene() const { return m_editorScene; }
    
    void reset();
    bool sceneIsType(SceneType type) const;
    void openScene(const fs::path &path);
    void closeScene(const fs::path &path);
    void saveActiveScene();
    void saveAll();

    Ref<Scene> cloneScene(const Ref<Scene> &scene);
  
  private:
    SceneManager() = default;
    virtual ~SceneManager() = default;

  public:
    void enterPlayMode();
    void exitPlayMode();

  private:
    Ref<Scene> m_gameScene;
    Ref<Scene> m_editorScene;
    bool m_inPlayMode = false;
};
} // namespace sky