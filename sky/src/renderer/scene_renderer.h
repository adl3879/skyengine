#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include "renderer/passes/forward_renderer.h"
#include "graphics/vulkan/vk_device.h"

namespace sky
{
class SceneRenderer
{
  public:
    SceneRenderer();
    ~SceneRenderer();

    SceneRenderer(SceneRenderer &) = delete;
    SceneRenderer operator=(SceneRenderer &) = delete;

    void render();

  private:
    void init();

  private:
    Ref<gfx::Device> m_device;

  private:
    ForwardRendererPass m_forwardRenderer;
};
} // namespace sky
