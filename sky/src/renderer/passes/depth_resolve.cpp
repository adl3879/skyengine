#include "depth_resolve.h"

#include "graphics/vulkan/vk_pipelines.h"

namespace sky
{
void DepthResolvePass::init(const gfx::Device &device, VkFormat format)
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/fullscreen_triangle.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/depth_resolve.frag.spv", device.getDevice());

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
                         .setDepthFormat(format)
                         .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
                         .build(device.getDevice());
}

void DepthResolvePass::draw(gfx::Device &device, 
    gfx::CommandBuffer cmd,
    ImageID depthImageId,
    int numOfSamples)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

    const auto &depthImage = device.getImage(depthImageId);

    // Set push constants
    const auto pushConstants = PushConstants{
        .depthImageSize = {
            static_cast<float>(depthImage.getExtent2D().width), 
            static_cast<float>(depthImage.getExtent2D().height), 
        },
        .depthImageId = depthImageId,
        .numSamples = numOfSamples,
    };

    vkCmdPushConstants(cmd, 
                      m_pInfo.pipelineLayout, 
                      VK_SHADER_STAGE_FRAGMENT_BIT, 
                      0, 
                      sizeof(PushConstants), 
                      &pushConstants);

    // Draw fullscreen triangle (3 vertices)
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void DepthResolvePass::cleanup(const gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
} // namespace sky