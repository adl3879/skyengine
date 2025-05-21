#pragma once

#include <skypch.h>

#include "renderer/passes/pass.h"

namespace sky 
{
class FormatConverterPass : public Pass
{
  public:
    void init(const gfx::Device &device, VkFormat format);
    void draw(
        gfx::Device &device, 
        gfx::CommandBuffer cmd,
        ImageID hdrImage, 
        VkExtent2D extent);
    void cleanup(const gfx::Device &device);

  private:
    struct PushConstants
    {
        ImageID hdrImage;
    };
};
}