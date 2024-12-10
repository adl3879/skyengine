#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include "renderer/passes/forward_renderer.h"
#include "graphics/vulkan/vk_device.h"
#include "scene/scene.h"
#include "material_cache.h"
#include "core/transform/transform.h"

namespace sky
{
class SceneRenderer
{
  public:
    SceneRenderer(gfx::Device &device);
    ~SceneRenderer();

    SceneRenderer(SceneRenderer &) = delete;
    SceneRenderer operator=(SceneRenderer &) = delete;

    void init(glm::ivec2 size);
    void render(gfx::CommandBuffer cmd, Ref<Scene> scene);
    void addDrawCommand(MeshDrawCommand drawCmd);
    
    MeshID addMeshToCache(const Mesh &mesh);
    MaterialID addMaterialToCache(const Material &material);
    ImageID createImage(const gfx::vkutil::CreateImageInfo& createInfo, void* pixelData);

    ImageID getDrawImageId() const { return m_drawImageID; }
    gfx::Device &getDevice() const { return m_device; }

    gfx::AllocatedImage getDrawImage();

  private:
    void destroy();
    void createDrawImage(glm::ivec2 size);
    void initSceneData();
    void updateLights(Ref<Scene> scene);

  private:
    gfx::Device &m_device;
    Ref<Scene> m_scene;
    std::vector<MeshDrawCommand> m_meshDrawCommands;

    MeshCache m_meshCache;
    MaterialCache m_materialCache;

  private:
    struct GPUSceneData
    {
        // camera
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        glm::vec4 cameraPos;

        // ambient
        LinearColorNoAlpha ambientColor;
        float ambientIntensity;

        VkDeviceAddress lightsBuffer;
        std::uint32_t numLights;
        std::int32_t sunlightIndex;

        VkDeviceAddress materialsBuffer;
    };

    gfx::NBuffer m_sceneDataBuffer; 

  private:
    ForwardRendererPass m_forwardRenderer;

    ImageID m_drawImageID{NULL_IMAGE_ID};
    ImageID m_depthImageID{NULL_IMAGE_ID};

    VkFormat m_drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat m_depthImageFormat{VK_FORMAT_D32_SFLOAT};
    VkSampleCountFlagBits m_samples{VK_SAMPLE_COUNT_1_BIT};
};
} // namespace sky
