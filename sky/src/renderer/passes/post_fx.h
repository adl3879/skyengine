#pragma once

#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"
#include <skypch.h>

namespace sky 
{
class PostFXPass : public Pass
{
  public:
    void init(const gfx::Device &device, VkFormat format);
    void draw(
        gfx::Device &device,
        gfx::CommandBuffer cmd,
        ImageID drawImageId,
        ImageID depthImageId,
        const gfx::AllocatedBuffer &sceneDataBuffer); 
    void cleanup(const gfx::Device &device);
  
  private:
    struct PushConstants {
        VkDeviceAddress sceneDataBuffer;
        std::uint32_t drawImageId;
        std::uint32_t depthImageId;
    };
};
}