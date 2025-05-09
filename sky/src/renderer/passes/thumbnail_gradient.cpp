#include "thumbnail_gradient.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky
{
void ThumbnailGradientPass::init(const gfx::Device &device, VkFormat format) 
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/thumbnail_gradient.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/thumbnail_gradient.frag.spv", device.getDevice());
    
    const auto pushConstantRanges = std::array<VkPushConstantRange, 0>{};
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
                           .setColorAttachmentFormat(format)
                           .build(device.getDevice());
}

void ThumbnailGradientPass::draw(gfx::Device &device, gfx::CommandBuffer cmd, VkExtent2D extent) 
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

    vkCmdDraw(cmd, 4, 1, 0, 0);
}

void ThumbnailGradientPass::cleanup(const gfx::Device &device) 
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}