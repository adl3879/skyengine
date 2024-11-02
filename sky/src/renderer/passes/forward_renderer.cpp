#include "forward_renderer.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky
{
void ForwardRendererPass::init(const gfx::Device &device) 
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/triangle.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/triangle.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{device.getBindlessDescSetLayout()};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
                         .setShaders(vertexShader, fragShader)
                         .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                         .setPolygonMode(VK_POLYGON_MODE_FILL)
                         .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                         .setMultisamplingNone()
                         .disableBlending()
                         .enableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL)
                         .setColorAttachmentFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
                         .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                         .build(device.getDevice());
}

void ForwardRendererPass::draw(
    const gfx::Device &device,
    gfx::CommandBuffer cmd, 
    VkExtent2D extent,
    const Camera &camera,
    const MeshCache &meshCache,
    const std::vector<MeshDrawCommand> &drawCommands)
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

    for (const auto &drawCommand : drawCommands)
	{
		const auto &mesh = meshCache.getMesh(drawCommand.meshId);

		const auto pushConstants = PushConstants{
			.worldMatrix = camera.getViewProjection(),
			.vertexBuffer = mesh.vertexBufferAddress,
		};

		vkCmdPushConstants(cmd, m_pInfo.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 
            sizeof(PushConstants), &pushConstants);
		vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);
	}
}

void ForwardRendererPass::cleanup(const gfx::Device &device) 
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
} // namespace sky