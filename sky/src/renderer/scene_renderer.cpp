#include "scene_renderer.h"

#include "core/application.h"
#include "graphics/vulkan/vk_images.h"

namespace sky
{
SceneRenderer::SceneRenderer(gfx::Device &device, Scene &scene)
	: m_device(device), m_scene(scene)
{
}

SceneRenderer::~SceneRenderer() {}

void SceneRenderer::init(glm::ivec2 size) 
{
    createDrawImage(size);

    m_forwardRenderer.init(m_device);
}

MeshId SceneRenderer::addMeshToCache(const Mesh& mesh)
{
    return m_meshCache.addMesh(m_device, mesh);
}

void SceneRenderer::destroy() 
{
    m_forwardRenderer.cleanup(m_device);
}

void SceneRenderer::createDrawImage(glm::ivec2 size)
{
    const auto drawImageExtent = VkExtent3D{
        .width = (std::uint32_t)size.x,
        .height = (std::uint32_t)size.y,
        .depth = 1,
    };

    { // setup draw image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        auto createImageInfo = gfx::vkutil::CreateImageInfo{
            .format = m_drawImageFormat,
            .usage = usages,
            .extent = drawImageExtent,
            .samples = m_samples,
        };
        // reuse the same id if creating again
        m_drawImageID = m_device.createImage(createImageInfo);
    }

    { // setup depth image
        auto createInfo = gfx::vkutil::CreateImageInfo{
            .format = m_depthImageFormat,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = drawImageExtent,
            .samples = m_samples,
        };

        // reuse the same id if creating again
        m_depthImageID = m_device.createImage(createInfo);
    }
}

gfx::AllocatedImage SceneRenderer::getDrawImage() 
{
	return m_device.getImage(m_drawImageID);
}

void SceneRenderer::addDrawCommand(MeshDrawCommand drawCmd)
{
    m_meshDrawCommands.push_back(drawCmd);
}

void SceneRenderer::render(gfx::CommandBuffer cmd) 
{
    auto drawImage = m_device.getImage(m_drawImageID);
    auto depthImage = m_device.getImage(m_depthImageID);

    const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 1.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 0.f,
        //.resolveImageView = isMultisamplingEnabled() ? resolveImage.imageView : VK_NULL_HANDLE,
    });

    gfx::vkutil::transitionImage(cmd, 
        drawImage.image, 
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    m_forwardRenderer.draw(
        m_device,
        cmd,
        drawImage.getExtent2D(),
        m_scene.getCamera(),
        m_meshCache,
        m_meshDrawCommands);

    vkCmdEndRendering(cmd);
}
} // namespace sky