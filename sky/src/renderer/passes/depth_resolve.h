#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "renderer/passes/pass.h"

namespace sky 
{
class DepthResolvePass : public Pass
{
  public:
    void init(const gfx::Device &device, VkFormat format);
    void draw(gfx::Device &device, 
        gfx::CommandBuffer cmd,
		ImageID depthImageId,
        int numOfSamples);
    void cleanup(const gfx::Device &device);

  private:
    struct PushConstants {
        glm::vec2 depthImageSize;
        std::uint32_t depthImageId;
        std::int32_t numSamples;
    };
};
}