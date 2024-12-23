#include "infinite_grid.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky
{
void InfiniteGridPass::init(const gfx::Device &device) 
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/grid.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/grid.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array<VkDescriptorSetLayout, 0>{};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
                           .setShaders(vertexShader, fragShader)
                           .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
                           .setPolygonMode(VK_POLYGON_MODE_FILL)
                           .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                           .setMultisamplingNone()
                           .enableBlending()
                           .disableDepthTest()
                           .setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                           .build(device.getDevice());
}

void InfiniteGridPass::draw(gfx::Device &device, 
	gfx::CommandBuffer cmd, 
    VkExtent2D extent,
	const gfx::AllocatedBuffer &sceneDataBuffer) 
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);

	 // set dynamic viewport and scissor
    const auto viewport = VkViewport{
        .x = 0.f,
		.y = 0.f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    const auto pcs = PushConstants{
        .sceneDataBuffer = sceneDataBuffer.address,
    };
    vkCmdPushConstants(cmd, 
        m_pInfo.pipelineLayout, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(PushConstants), 
        &pcs);

    // No vertex buffer binding required
    vkCmdDraw(cmd, /*vertexCount=*/6, /*instanceCount=*/1, /*firstVertex=*/0, /*firstInstance=*/0);
}

void InfiniteGridPass::cleanup(const gfx::Device &device) 
{
	vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
} // namespace sky