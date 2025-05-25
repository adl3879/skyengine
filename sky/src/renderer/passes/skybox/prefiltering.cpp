#include "graphics/vulkan/vk_pipelines.h"
#include "prefilering.h"
#include "core/application.h"

namespace sky 
{
void PrefilterEnvmapPass::init(gfx::Device& device, VkFormat format, uint32_t baseSize, uint32_t mipLevels)
{
    auto vertShader = gfx::vkutil::loadShaderModule("shaders/cubemap.vert.spv", device.getDevice());
    auto fragShader = gfx::vkutil::loadShaderModule("shaders/prefilter_envmap.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array{device.getBindlessDescSetLayout()};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

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

    auto imageInfo = gfx::vkutil::CreateImageInfo{
        .format = format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .extent = {baseSize, baseSize, 1},
        .numLayers = 6,
        .numMipLevels = mipLevels,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .mipMap = true,
        .isCubemap = true,
    };

    m_prefilteredMapId = device.createImage(imageInfo);

    for (uint32_t mip = 0; mip < mipLevels; ++mip)
    {
        for (uint32_t face = 0; face < 6; ++face) 
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = device.getImage(m_prefilteredMapId).image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = mip;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = face;
            viewInfo.subresourceRange.layerCount = 1;

            uint32_t index = mip * 6 + face;
            if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &m_faceMipViews[index]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create cubemap prefiltered face view");
            }
        }
    }
}
    
void PrefilterEnvmapPass::draw(gfx::Device& device, gfx::CommandBuffer cmd, ImageID environmentMap)
{
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1.0f; // flip Y for Vulkan

    const uint32_t mipLevels = 5;
    const uint32_t baseSize = device.getImage(m_prefilteredMapId).imageExtent.width;
    const uint32_t numSamples = 1024;

    // FOR CUBE
    auto renderer = Application::getRenderer();
    auto meshCache = renderer->getMeshCache();
    auto mesh = meshCache.getMesh(renderer->getCubeMesh());

    for (uint32_t mip = 0; mip < mipLevels; ++mip) 
    {
        uint32_t mipWidth  = std::max(1u, baseSize >> mip);
        uint32_t mipHeight = std::max(1u, baseSize >> mip);
        float roughness = (float)mip / (float)(mipLevels - 1);

        for (uint32_t face = 0; face < 6; ++face) 
        {
            auto& view = m_faceMipViews[mip * 6 + face];

            auto renderingInfo = gfx::vkutil::createRenderingInfo({
                .renderExtent = { mipWidth, mipHeight },
                .colorImageView = view,
            });

            vkCmdBeginRendering(cmd, &renderingInfo.renderingInfo); 
            
            PushConstants pc{};
            pc.view = captureViews[face];
            pc.proj = proj;
            pc.roughness = roughness;
            pc.numSamples = numSamples;
            pc.envMapId = environmentMap;
            pc.vertexBuffer = mesh.vertexBuffer.address;
            
            vkCmdPushConstants(cmd, m_pInfo.pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(PushConstants), &pc);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
            
            VkDescriptorSet sets[] = { device.getBindlessDescSet() };
            device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, sets);

            gfx::vkutil::setViewportAndScissor(cmd, {mipWidth, mipHeight});
                
            vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);

            vkCmdEndRendering(cmd);
        }
    }
}


void PrefilterEnvmapPass::cleanup(gfx::Device& device)
{
    for (auto& view : m_faceMipViews)
        if (view)
            vkDestroyImageView(device.getDevice(), view, nullptr);

    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}