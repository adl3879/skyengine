#include "format_converter.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky 
{
void FormatConverterPass::init(const gfx::Device &device, VkFormat format)
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/format_converter.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
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
        .disableCulling()
        .setMultisamplingNone()
        .disableDepthTest()
        .disableBlending()
        .setColorAttachmentFormat(format)
        .build(device.getDevice());
}

void FormatConverterPass::draw(
    gfx::Device &device, 
    gfx::CommandBuffer cmd, 
    ImageID hdrImage, 
    VkExtent2D extent)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

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

    // Set push constants
    const auto pushConstants = PushConstants{
       .hdrImage = hdrImage, 
    };

    vkCmdPushConstants(cmd, 
        m_pInfo.pipelineLayout, 
        VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(PushConstants), 
        &pushConstants);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void FormatConverterPass::cleanup(const gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}