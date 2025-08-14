#include "irradiance.h"
#include "core/application.h"
#include "graphics/vulkan/vk_pipelines.h"

namespace sky 
{
void IrradiancePass::init(gfx::Device& device, VkFormat format, uint32_t size)
{
    // Load shaders
    auto vertShader = gfx::vkutil::loadShaderModule("shaders/cubemap.vert.spv", device.getDevice());
    auto fragShader = gfx::vkutil::loadShaderModule("shaders/irradiance.frag.spv", device.getDevice());

    // Push constants
    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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

    // Create irradiance cubemap (small resolution, no mipmaps)
    auto imageInfo = gfx::vkutil::CreateImageInfo{
        .format = format,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .extent = {size, size, 1},
        .numLayers = 6,
        .numMipLevels = 1, // No mipmaps for irradiance
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .mipMap = false,
        .isCubemap = true,
    };

    m_irradianceMapId = device.createImage(imageInfo);

    // Create image views for each face
    for (uint32_t face = 0; face < 6; ++face) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = device.getImage(m_irradianceMapId).image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &m_faceViews[face]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create irradiance cubemap face view");
        }
    }
}

void IrradiancePass::reset(gfx::Device &device, gfx::CommandBuffer cmd)
{
    auto image = device.getImage(m_irradianceMapId);
    gfx::vkutil::clearColorImage(cmd, image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void IrradiancePass::draw(gfx::Device& device, gfx::CommandBuffer cmd, ImageID environmentMap)
{
    // Standard cubemap face orientations
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),   // +X
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),  // -X
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),    // +Y
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),   // -Y
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),   // +Z
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))   // -Z
    };
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    proj[1][1] *= -1.0f; // flip Y for Vulkan

    const uint32_t irradianceSize = device.getImage(m_irradianceMapId).imageExtent.width;

    // Get cube mesh
    auto renderer = Application::getRenderer();
    auto meshCache = renderer->getMeshCache();
    auto mesh = meshCache.getMesh(renderer->getCubeMesh());

    // Render each face
    for (uint32_t face = 0; face < 6; ++face) 
    {
        auto renderingInfo = gfx::vkutil::createRenderingInfo({
            .renderExtent = { irradianceSize, irradianceSize },
            .colorImageView = m_faceViews[face],
        });

        vkCmdBeginRendering(cmd, &renderingInfo.renderingInfo);
        
        // Set push constants
        PushConstants pc{};
        pc.view = captureViews[face];
        pc.proj = proj;
        pc.vertexBuffer = mesh.vertexBuffer.address;
        pc.envMapId = environmentMap;
        
        vkCmdPushConstants(cmd, m_pInfo.pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PushConstants), &pc);

        // Bind pipeline and resources
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
        
        VkDescriptorSet sets[] = { device.getBindlessDescSet() };
        device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, sets);

        gfx::vkutil::setViewportAndScissor(cmd, {irradianceSize, irradianceSize});
            
        // Draw cube
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.numIndices, 1, 0, 0, 0);

        vkCmdEndRendering(cmd);
    }
}

void IrradiancePass::cleanup(gfx::Device& device)
{
    // Clean up face views
    for (auto& view : m_faceViews) {
        if (view) {
            vkDestroyImageView(device.getDevice(), view, nullptr);
        }
    }

    // Clean up pipeline resources
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}
}