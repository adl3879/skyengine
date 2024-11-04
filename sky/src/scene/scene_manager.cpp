#include "scene_manager.h"

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
    m_editorScene = CreateRef<Scene>();
    m_activeScene = m_editorScene;
}
} // namespace sky