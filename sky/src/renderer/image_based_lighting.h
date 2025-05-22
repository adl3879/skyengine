#pragma once

#include "graphics/vulkan/vk_device.h"
#include "graphics/vulkan/vk_types.h"
#include "passes/equirectangular_to_cubemap.h"
#include "passes/skybox.h"
#include <skypch.h>

namespace sky 
{
class ImageBasedLighting
{
  public:
    void init(gfx::Device &device);
    void draw(gfx::Device &device, gfx::CommandBuffer cmd, const gfx::AllocatedBuffer &sceneDataBuffer);
    void drawSky(gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        const gfx::AllocatedBuffer &sceneDataBuffer);
    void cleanup(gfx::Device &device);

  private:
    EquirectangularToCubemapPass m_equirectangularToCubemapPass;
    SkyboxPass m_skyboxPass;

    bool m_dirty{true};
    ImageID m_hdrImageId;
};
}