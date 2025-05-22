#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"

namespace sky 
{
class EquirectangularToCubemapPass : public Pass
{
  public:
    void init(gfx::Device &device, VkFormat format, VkExtent2D extent);
    void draw(gfx::Device &device, gfx::CommandBuffer cmd, ImageID hdrImage, VkExtent2D extent);
    void cleanup(gfx::Device &device);

    auto getCubemapImageId() const { return m_cubemapImageId; }

  private:
    VkImageView getCubemapFaceView(uint32_t face) const { return m_cubemapFaceViews[face]; }

    ImageID m_cubemapImageId;
    VkImageView m_cubemapFaceViews[6];
  
  private:
    struct PushConstants 
    {
        glm::mat4 view;
        glm::mat4 proj;
        ImageID hdrImage;
    };
};
}
