#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"

namespace sky 
{
class PrefilterEnvmapPass : public Pass 
{
  public:
    void init(gfx::Device& device, VkFormat format, uint32_t baseSize, uint32_t mipLevels);
    void draw(gfx::Device& device, gfx::CommandBuffer cmd, ImageID environmentMap);
    void cleanup(gfx::Device& device);
    void reset() { m_prefilteredMapId = NULL_IMAGE_ID; }

    ImageID getCubemapId() const { return m_prefilteredMapId; }

  private:
    struct PushConstants
    {
        glm::mat4 view;
        glm::mat4 proj;
        VkDeviceAddress vertexBuffer;
        float roughness;
        uint32_t numSamples; 
        ImageID envMapId;
    };

    ImageID m_prefilteredMapId;
    std::array<VkImageView, 6 * 5> m_faceMipViews; // for 5 mip levels × 6 faces
};      
}