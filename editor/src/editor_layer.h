#pragma once

#include <core/layer.h>
#include <skypch.h>

#include "graphics/vulkan/vk_device.h"
#include "renderer/scene_renderer.h"
#include "scene/scene.h"

namespace sky
{
class EditorLayer : public Layer
{
  public:
    EditorLayer();
    ~EditorLayer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(Event &e) override;
    void onFixedUpdate(float dt) override;
    void onImGuiRender() override;

  private:
    Ref<gfx::Device> m_gfxDevice;
    Ref<SceneRenderer> m_renderer;

    Ref<Scene> m_activeScene;
}; 
} // namespace sky
