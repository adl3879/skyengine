#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_device.h"
#include "sprite.h"
#include "passes/pass.h"

namespace sky
{
const uint32_t MAX_SPRITES = 1000;
const uint32_t VERTICES_PER_QUAD = 4;
const uint32_t INDICES_PER_QUAD = 6;

// Total buffer sizes
const uint32_t MAX_VERTICES = MAX_SPRITES * VERTICES_PER_QUAD;
const uint32_t MAX_INDICES = MAX_SPRITES * INDICES_PER_QUAD;

class SpriteBatchRenderer : public Pass
{
  public:
	void init(gfx::Device &device, VkFormat format);
    void cleanup(gfx::Device &device);

	void drawSprite(gfx::Device &device, const Sprite &sprite);
    void flush(gfx::Device &device, 
		gfx::CommandBuffer cmd,
		VkExtent2D extent, 	
		const gfx::AllocatedBuffer &sceneDataBuffer);

  private:
	std::array<glm::vec2, 4> calculateTransformedVertices(const Sprite &sprite);
    void uploadBuffers(gfx::Device &device);

  private:
	gfx::AllocatedBuffer m_vertexBuffer;
	gfx::AllocatedBuffer m_indexBuffer;
	gfx::AllocatedBuffer m_stagingBuffer;

	std::vector<QuadVertex> m_vertices;
	std::vector<uint32_t> m_indices;

	uint32_t m_currentVertexCount;

  private:
	struct PushConstants
    {
		VkDeviceAddress sceneDataBuffer;
        VkDeviceAddress vertexBuffer;
    };
};
}