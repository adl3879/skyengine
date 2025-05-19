#pragma once

#include <skypch.h>
#include <vulkan/vulkan_core.h>

#include "passes/post_fx.h"
#include "passes/skybox.h"
#include "renderer/passes/depth_resolve.h"
#include "renderer/camera/camera.h"
#include "renderer/passes/forward_renderer.h"
#include "renderer/passes/infinite_grid.h"
#include "graphics/vulkan/vk_device.h"
#include "scene/scene.h"
#include "material_cache.h"
#include "renderer/passes/sky_atmosphere.h"
#include "sprite_renderer.h"

namespace sky
{
class SceneRenderer
{
  public:    
    static constexpr VkSampleCountFlagBits MAX_SAMPLES = VK_SAMPLE_COUNT_1_BIT;

  public:
    SceneRenderer(gfx::Device &device);
    ~SceneRenderer();

    SceneRenderer(SceneRenderer &) = delete;
    SceneRenderer operator=(SceneRenderer &) = delete;

    void init(glm::ivec2 size);
    void render(gfx::CommandBuffer &cmd, Ref<Scene> scene, Camera &cam, ImageID drawImage);
    void update(Ref<Scene> scene);

    void initBuiltins();
    void drawMesh(MeshID id, const glm::mat4 &transform, bool visibility, uint32_t uniqueId, MaterialID mat = NULL_MATERIAL_ID);
    void drawModel(Ref<Model> model, const glm::mat4 &transform);
    void clearDrawCommands() { m_meshDrawCommands.clear(); }

    MeshID addMeshToCache(const Mesh &mesh);
    MaterialID addMaterialToCache(const Material &material);
    void updateMaterial(MaterialID id, Material material);
    ImageID createImage(const gfx::vkutil::CreateImageInfo &createInfo, void *pixelData);
    void addTempModel(const fs::path &path, Ref<Model> model);
    Ref<Model> getTempModel(const fs::path &path) { return m_tempModels.at(path); }
    bool isTempModelLoaded(const fs::path &path) { return m_tempModels.contains(path); }

    ImageID getGameDrawImageId() const { return m_gameDrawImageID; }
    gfx::Device &getDevice() const { return m_device; }
    const Mesh &getMesh(MeshID id) const { return m_meshCache.getCPUMesh(id); }
    const Material &getMaterial(MaterialID id) const { return m_materialCache.getMaterial(id); }
    auto getMeshCache() const { return m_meshCache; }
    auto getMaterialCache() const { return m_materialCache; }
    
    ImageID getCheckerboardTexture() const { return m_device.getCheckerboardTextureID(); }
    gfx::AllocatedImage getDrawImage();

    ImageID createNewDrawImage(glm::ivec2 size, VkFormat format, VkSampleCountFlagBits samples);
    ImageID createNewDepthImage(glm::ivec2 size, VkSampleCountFlagBits samples);
    
    ForwardRendererPass getForwardRendererPass() const { return m_forwardRenderer; }
    auto getSphereMesh() const { return m_builtinModels.at(ModelType::Sphere); }

    // Final draw image from the scene renderer
    ImageID getSceneImage() const { return m_postFXImageID; }

  private:
    void destroy();
    void createDrawImage(glm::ivec2 size);
    void initSceneData();
    void updateLights(Ref<Scene> scene);
    void mousePicking(Ref<Scene> scene);
    bool isMultisamplingEnabled() const;

  private:
    gfx::Device &m_device;
    Ref<Scene> m_scene;
    std::vector<MeshDrawCommand> m_meshDrawCommands;

    MeshCache m_meshCache;
    MaterialCache m_materialCache;

  public:
    struct GPUSceneData
    {
        // camera
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewProj;
        glm::vec4 cameraPos;
        glm::vec2 mousePos;
        glm::vec2 viewportSize;

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
    InfiniteGridPass m_infiniteGridPass;
    SkyAtmospherePass m_skyAtmospherePass;
    SpriteBatchRenderer m_spriteRenderer;
    DepthResolvePass m_depthResolvePass;
    PostFXPass m_postFXPass;
    // SkyboxPass m_skyboxPass;

    ImageID m_drawImageID{NULL_IMAGE_ID};
    ImageID m_gameDrawImageID{NULL_IMAGE_ID};
    ImageID m_depthImageID{NULL_IMAGE_ID};
    ImageID m_resolveImageID{NULL_IMAGE_ID};
    ImageID m_resolveDepthImageID{NULL_IMAGE_ID};
    ImageID m_postFXImageID{NULL_IMAGE_ID};

    VkFormat m_drawImageFormat{VK_FORMAT_R16G16B16A16_SFLOAT};
    VkFormat m_depthImageFormat{VK_FORMAT_D32_SFLOAT};
    VkSampleCountFlagBits m_samples{MAX_SAMPLES};

    std::unordered_map<ModelType, MeshID> m_builtinModels;
    std::unordered_map<fs::path, Ref<Model>> m_tempModels;
};
} // namespace sky
