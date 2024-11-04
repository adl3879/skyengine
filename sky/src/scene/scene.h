#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "renderer/camera/perspective_camera.h"
#include "renderer/camera/editor_camera.h"

namespace sky
{
class Entity;

class Scene
{
  public:
    Scene(const std::string &name = "Untitled");
    ~Scene() = default;

    void init();
    void update(float dt);
    void cleanup();

    Entity createEntity(const std::string &name = std::string());
    void destroyEntity(Entity entity);
    Camera &getCamera();
    EditorCamera &getEditorCamera() { return m_editorCamera; }
    //void setMainCamera(Camera &camera) { m_mainCamera = camera; }

  private:
    void newScene(const std::string &name);

  private:
    friend class Entity;
    entt::registry m_registry;
    std::string m_sceneName;

    PerspectiveCamera m_mainCamera;
    EditorCamera m_editorCamera;
};
}