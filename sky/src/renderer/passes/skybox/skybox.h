#pragma once

#include <skypch.h>

#include "renderer/passes/pass.h"

namespace sky
{
class SkyboxPass : public Pass
{
  public:
    void init(gfx::Device &device, VkFormat format);
    void draw(gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        ImageID cubemapImage, 
        const gfx::AllocatedBuffer &sceneDataBuffer);
    void cleanup(gfx::Device &device);

  private:
    struct PushConstants 
    {
        VkDeviceAddress sceneDataBuffer;
        ImageID cubemapImage;
    };
};
}