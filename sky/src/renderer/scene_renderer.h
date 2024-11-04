#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include "renderer/passes/forward_renderer.h"
#include "graphics/vulkan/vk_device.h"
#include "scene/scene.h"

namespace sky
{
class SceneRenderer
{
  public:
    SceneRenderer(gfx::Device &device, Scene &scene);
    ~SceneRenderer();

    SceneRenderer(SceneRenderer &) = delete;
    SceneRenderer operator=(SceneRenderer &) = delete;

    void init(glm::ivec2 size);
    void render(gfx::CommandBuffer cmd);
    void addDrawCommand(MeshDrawCommand drawCmd);
    MeshId addMeshToCache(const Mesh &mesh);

    gfx::AllocatedImage getDrawImage();

  private:
    void destroy();
    void createDrawImage(glm::ivec2 size);

  private:
    gfx::Device &m_device;
    Scene &m_scene;
    MeshCache m_meshCache;
    std::vector<MeshDrawCommand> m_meshDrawCommands;

  private:
    ForwardRendererPass m_forwardRenderer;

    ImageID m_drawImageID{NULL_IMAGE_ID};
    ImageID m_depthImageID{NULL_IMAGE_ID};

    VkFormat m_drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat m_depthImageFormat{VK_FORMAT_D32_SFLOAT};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
};
} // namespace sky
