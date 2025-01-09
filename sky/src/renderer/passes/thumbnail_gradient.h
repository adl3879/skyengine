#pragma once

#include <skypch.h>
#include "pass.h"

namespace sky
{
class ThumbnailGradientPass : public Pass
{
  public:
	ThumbnailGradientPass() = default;
	~ThumbnailGradientPass() = default;

	void init(const gfx::Device &device, VkFormat format);
	void draw(gfx::Device &device, 
        gfx::CommandBuffer cmd,
		VkExtent2D extent);
    void cleanup(const gfx::Device &device);
};
}