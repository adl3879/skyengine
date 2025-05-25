#pragma once

#include <skypch.h>
#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"

namespace sky 
{
class IrradiancePass : public Pass 
{
public:
    void init(gfx::Device& device, VkFormat format, uint32_t size = 32);
    void draw(gfx::Device& device, gfx::CommandBuffer cmd, ImageID environmentMap);
    void cleanup(gfx::Device& device);
    
    ImageID getCubemapId() const { return m_irradianceMapId; }

private:
    struct PushConstants
    {
        glm::mat4 view;
        glm::mat4 proj;
        uint64_t vertexBuffer;
        ImageID envMapId;
    };

    ImageID m_irradianceMapId;
    std::array<VkImageView, 6> m_faceViews; // 6 faces, no mip levels needed
};
}