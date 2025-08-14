#include "brdf_lut.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky 
{
void BrdfLutPass::init(gfx::Device& device, VkFormat format, uint32_t size)
{
    // Load shaders
    auto vertShader = gfx::vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device.getDevice());
    auto fragShader = gfx::vkutil::loadShaderModule("shaders/brdf_lut.frag.spv", device.getDevice());

    // Push constants
    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{device.getBindlessDescSetLayout()};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    // Create pipeline
    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
        .setShaders(vertShader, fragShader)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .disableBlending()
        .disableDepthTest()
        .disableCulling()
        .setMultisamplingNone()
        .setColorAttachmentFormat(format)
        .build(device.getDevice());

    // Create BRDF LUT texture (2D, single mip level)
    auto imageInfo = gfx::vkutil::CreateImageInfo{
        .format = format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags = 0, // Not a cubemap
        .extent = {size, size, 1},
        .numLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
    };

    m_brdfLutId = device.createImage(imageInfo);
}

void BrdfLutPass::reset(gfx::Device &device, gfx::CommandBuffer cmd)
{
    auto image = device.getImage(m_brdfLutId);
    gfx::vkutil::clearColorImage(cmd, image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void BrdfLutPass::draw(gfx::Device& device, gfx::CommandBuffer cmd)
{
    const uint32_t lutSize = device.getImage(m_brdfLutId).imageExtent.width;
    auto brdfLutImage = device.getImage(m_brdfLutId);

    auto renderingInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = { lutSize, lutSize },
        .colorImageView = brdfLutImage.imageView,
    });

    vkCmdBeginRendering(cmd, &renderingInfo.renderingInfo);
    
    // Set push constants
    PushConstants pc{};
    pc.size = lutSize;
    
    vkCmdPushConstants(cmd, m_pInfo.pipelineLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(PushConstants), &pc);

    // Bind pipeline and resources
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    
    VkDescriptorSet sets[] = { device.getBindlessDescSet() };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, sets);

    gfx::vkutil::setViewportAndScissor(cmd, {lutSize, lutSize});
        
    // Draw fullscreen triangle
    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

void BrdfLutPass::cleanup(gfx::Device& device)
{
    // Clean up pipeline resources
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}