#pragma once

#include <skypch.h>
#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"

namespace sky 
{
class BrdfLutPass : public Pass 
{
  public:
    void init(gfx::Device& device, VkFormat format, uint32_t size = 512);
    void draw(gfx::Device& device, gfx::CommandBuffer cmd);
    void cleanup(gfx::Device& device);
    void reset() { m_brdfLutId = NULL_IMAGE_ID; }
    
    ImageID getLutId() const { return m_brdfLutId; }

  private:
    struct PushConstants
    {
        uint32_t size;
    };

    ImageID m_brdfLutId;
};
}