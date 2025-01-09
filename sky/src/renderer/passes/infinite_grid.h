#pragma once

#include <skypch.h>
#include "pass.h"

namespace sky
{
class InfiniteGridPass : public Pass
{
  public:
	InfiniteGridPass() = default;
	~InfiniteGridPass() = default;

	void init(const gfx::Device &device, VkFormat format);
	void draw(gfx::Device &device, 
        gfx::CommandBuffer cmd,
		VkExtent2D extent,
        const gfx::AllocatedBuffer &sceneDataBuffer);
    void cleanup(const gfx::Device &device);

  private:
	struct PushConstants
	{
        VkDeviceAddress sceneDataBuffer;
	};
};
}

