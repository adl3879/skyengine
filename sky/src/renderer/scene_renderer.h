#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include "renderer/passes/forward_renderer.h"
#include "graphics/vulkan/vk_device.h"
#include "scene/scene.h"
#include "material_cache.h"

namespace sky
{
class SceneRenderer
{
  public:
    SceneRenderer(gfx::Device &device);
    ~SceneRenderer();

    SceneRenderer(SceneRenderer &) = delete;
    SceneRenderer operator=(SceneRenderer &) = delete;

    void init(Ref<Scene> scene, glm::ivec2 size);
    void render(gfx::CommandBuffer cmd);
    void addDrawCommand(MeshDrawCommand drawCmd);
    
    MeshID addMeshToCache(const Mesh &mesh);
    MaterialID addMaterialToCache(const Material &material);
    ImageID createImage(const gfx::vkutil::CreateImageInfo& createInfo, void* pixelData);

    ImageID getDrawImageId() const { return m_drawImageID; }

    gfx::AllocatedImage getDrawImage();

  private:
    void destroy();
    void createDrawImage(glm::ivec2 size);

  private:
    gfx::Device &m_device;
    Ref<Scene> m_scene;
    std::vector<MeshDrawCommand> m_meshDrawCommands;

    MeshCache m_meshCache;
    MaterialCache m_materialCache;

  private:
    ForwardRendererPass m_forwardRenderer;

    ImageID m_drawImageID{NULL_IMAGE_ID};
    ImageID m_depthImageID{NULL_IMAGE_ID};

    VkFormat m_drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat m_depthImageFormat{VK_FORMAT_D32_SFLOAT};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
};
} // namespace sky
