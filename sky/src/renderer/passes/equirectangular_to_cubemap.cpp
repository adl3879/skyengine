#include "equirectangular_to_cubemap.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "graphics/vulkan/vk_pipelines.h"
#include "graphics/vulkan/vk_utils.h"

namespace sky 
{
void EquirectangularToCubemapPass::init(gfx::Device &device, VkFormat format, VkExtent2D extent)
{
    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/equirect_to_cubemap.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/equirect_to_cubemap.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
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
        .disableCulling()
        .setMultisamplingNone()
        .setColorAttachmentFormat(format)
        .disableDepthTest()
        .build(device.getDevice());

    // Create a cubemap texture
    auto imageInfo = gfx::vkutil::CreateImageInfo{
        .format = format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .extent = {extent.width, extent.height, 1},
        .numLayers = 6,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .isCubemap = true,
    };

    m_cubemapImageId = device.createImage(imageInfo);
    
    // Create image views for each face of the cubemap
    for (uint32_t i = 0; i < 6; ++i) 
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = device.getImage(m_cubemapImageId).image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i;
        viewInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &m_cubemapFaceViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap face image view");
        }
    }
}

void EquirectangularToCubemapPass::draw(gfx::Device &device, 
    gfx::CommandBuffer cmd, 
    ImageID hdrImage, 
    VkExtent2D extent)
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
    
    for (uint32_t face = 0; face < 6; ++face) 
    {
        const auto renderInfo = gfx::vkutil::createRenderingInfo({
            .renderExtent = extent,
            .colorImageView = getCubemapFaceView(face),
        });
    
        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
    
        // 2. Push constants
        PushConstants pc{};
        pc.view = captureViews[face];
        pc.proj = proj;
        pc.hdrImage = hdrImage;
    
        vkCmdPushConstants(cmd, 
            m_pInfo.pipelineLayout, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            0, 
            sizeof(PushConstants), 
            &pc);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);

        VkDescriptorSet descriptorSets[] = { device.getBindlessDescSet() };
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
    
        // 4. Draw
        vkCmdDraw(cmd, 3, 1, 0, 0);
    
        // 5. End rendering
        vkCmdEndRendering(cmd);
    }
}

void EquirectangularToCubemapPass::cleanup(gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
    for (auto view : m_cubemapFaceViews)
    {
        vkDestroyImageView(device.getDevice(), view, nullptr);
    }
}
}