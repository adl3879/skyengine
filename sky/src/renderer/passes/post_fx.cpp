#include "post_fx.h"

#include "graphics/vulkan/vk_pipelines.h"
#include "graphics/vulkan/vk_types.h"

namespace sky 
{
void PostFXPass::init(const gfx::Device &device, VkFormat format)
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/post_fx.frag.spv", device.getDevice());

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

void PostFXPass::draw(
    gfx::Device &device,
    gfx::CommandBuffer cmd,
    ImageID drawImageId,
    ImageID depthImageId,
    const gfx::AllocatedBuffer &sceneDataBuffer)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);
    
    const auto &drawImage = device.getImage(drawImageId);
    const auto &depthImage = device.getImage(depthImageId);

    gfx::vkutil::setViewportAndScissor(cmd, drawImage.getExtent2D());

    // Set push constants
    const auto pushConstants = PushConstants{
        .sceneDataBuffer = sceneDataBuffer.address,
        .drawImageId = drawImageId,
        .depthImageId = depthImageId,
    };

    vkCmdPushConstants(cmd, 
        m_pInfo.pipelineLayout, 
        VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(PushConstants), 
        &pushConstants);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void PostFXPass::cleanup(const gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
} // namespace sky