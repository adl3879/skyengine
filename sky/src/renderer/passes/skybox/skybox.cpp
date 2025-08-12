#include "skybox.h"
#include "graphics/vulkan/vk_pipelines.h"

namespace sky
{
void SkyboxPass::init(gfx::Device &device, VkFormat format)
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/skybox.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/skybox.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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
        .disableBlending()
        .setCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .setMultisamplingNone()
        .setColorAttachmentFormat(format)
        .enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setDepthFormat(VK_FORMAT_D32_SFLOAT)
        .build(device.getDevice());
}

void SkyboxPass::draw(gfx::Device &device, 
    gfx::CommandBuffer cmd,
    VkExtent2D extent, 
    ImageID cubemapImage, 
    const gfx::AllocatedBuffer &sceneDataBuffer)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

    gfx::vkutil::setViewportAndScissor(cmd, extent);

    // Set push constants
    const auto pushConstants = PushConstants{
        .sceneDataBuffer = sceneDataBuffer.address,
        .cubemapImage = cubemapImage,
    };

    vkCmdPushConstants(cmd, 
        m_pInfo.pipelineLayout, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(PushConstants), 
        &pushConstants);

    // Draw a cube (36 vertices: 6 faces, 2 triangles per face, 3 vertices per triangle)
    vkCmdDraw(cmd, 36, 1, 0, 0);
}

void SkyboxPass::cleanup(gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}