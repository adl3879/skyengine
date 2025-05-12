#include "custom_thumbnail.h"

#include "core/application.h"
#include "graphics/vulkan/vk_images.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "renderer/passes/forward_renderer.h"
#include "scene/scene_manager.h"

#include "renderer/camera/camera.h"

#include <stb_image_write.h>
#include <vulkan/vulkan_core.h>

namespace sky
{
VkResult saveVkImageToPNG(gfx::Device &gfxDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage sourceImage,
                          uint32_t width, uint32_t height, const char *filename)
{
    // Create a buffer to store the image data
    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                     .size = width * height * 4, // Assuming RGBA8 format
                                     .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

    VkBuffer stagingBuffer;
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    if (result != VK_SUCCESS) return result;

    // Get memory requirements and allocate
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =  gfxDevice.getMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr)
    };

    VkDeviceMemory stagingMemory;
    result = vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
    if (result != VK_SUCCESS)
    {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return result;
    }

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // Create command buffer for copy operation
    VkCommandBufferAllocateInfo cmdBufInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                              .commandPool = commandPool,
                                              .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                              .commandBufferCount = 1};

    VkCommandBuffer cmdBuffer;
    result = vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuffer);
    if (result != VK_SUCCESS)
    {
        vkFreeMemory(device, stagingMemory, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return result;
    }

    // Begin command buffer recording
    VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // Transition image layout for transfer
    VkImageMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                    .srcAccessMask = 0,
                                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                                    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // Adjust based on current layout
                                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                    .image = sourceImage,
                                    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                         .baseMipLevel = 0,
                                                         .levelCount = 1,
                                                         .baseArrayLayer = 0,
                                                         .layerCount = 1},
    };

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    // Copy image to buffer
    VkBufferImageCopy region = {.bufferOffset = 0,
                                .bufferRowLength = 0,
                                .bufferImageHeight = 0,
                                .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                     .mipLevel = 0,
                                                     .baseArrayLayer = 0,
                                                     .layerCount = 1},
                                .imageOffset = {0, 0, 0},
                                .imageExtent = {width, height, 1}};

    vkCmdCopyImageToBuffer(cmdBuffer, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &region);

    // End and submit command buffer
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cmdBuffer};

    result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // Map memory and write to file
    void *data;
    vkMapMemory(device, stagingMemory, 0, VK_WHOLE_SIZE, 0, &data);

    // Write to PNG using stb_image_write
    stbi_write_png(filename, width, height, 4, data, width * 4);

    // Cleanup
    vkUnmapMemory(device, stagingMemory);
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
    vkFreeMemory(device, stagingMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);

    return VK_SUCCESS;
}

CustomThumbnail::CustomThumbnail() 
{
	auto &device = Application::getRenderer()->getDevice();

	m_lightCache.init(device);
	auto transform = Transform{};
	transform.rotateDegrees({120.f, 30.f, 0.f});
	m_lightCache.addLight(Light{
        .type = LightType::Directional, 
        .color = LinearColorNoAlpha::white(), 
        .intensity = 5.f}, transform);

    m_thumbnailGradientPass.init(device, m_drawImageFormat);
	m_forwardRenderer.init(device, m_drawImageFormat, VK_SAMPLE_COUNT_4_BIT);
}

CustomThumbnail::~CustomThumbnail()
{
    auto &device = Application::getRenderer()->getDevice();
    m_thumbnailGradientPass.cleanup(device);
}

void CustomThumbnail::render(gfx::CommandBuffer cmd) 
{
    while (!m_queue.empty())
    {
        auto path = m_queue.front();
        auto assetType = getAssetTypeFromFileExtension(path.extension());
        switch (assetType)
        {
            case AssetType::Material:
            {
                auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Material);
                auto materialId = AssetManager::getAsset<MaterialAsset>(handle)->material;
                generateMaterialThumbnail(cmd, materialId, path);
                break;
            }
            case AssetType::Mesh:
            {
                auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Mesh);
                /*AssetManager::getAssetAsync<Model>(handle, [&](const Ref<Model> &model) {
					generateModelThumbnail(cmd, model->meshes, path);
                });*/
                auto meshes = AssetManager::getAsset<Model>(handle)->meshes;
                generateModelThumbnail(cmd, meshes, path);
				break;
            }
            default: break;
        }

        m_queue.pop();
        break;
    }
}

ImageID CustomThumbnail::getOrCreateThumbnail(const fs::path &path) 
{
	if (m_thumbnails.contains(path)) return m_thumbnails.at(path).first;

	auto drawImage = Application::getRenderer()->createNewDrawImage(m_size, m_drawImageFormat);
	auto depthImage = Application::getRenderer()->createNewDepthImage(m_size);
	m_thumbnails[path] = std::make_pair(drawImage, depthImage);
	m_queue.push(path);

	return drawImage;
}

void CustomThumbnail::refreshThumbnail(const fs::path &path)
{
    if (m_thumbnails.contains(path)) m_thumbnails.erase(path);
}

void CustomThumbnail::generateMaterialThumbnail(gfx::CommandBuffer cmd, MaterialID mat, const fs::path &path) 
{
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();

    auto drawImage = device.getImage(m_thumbnails[path].first);
    auto depthImage = device.getImage(m_thumbnails[path].second);

    gfx::NBuffer sceneDataBuffer;
	sceneDataBuffer.init(
        device,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(SceneRenderer::GPUSceneData),
        gfx::FRAME_OVERLAP,
        "scene data");

    auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.8f),   // Camera position slightly back
                            glm::vec3(0.0f, 0.0f, 0.0f),   // Looking at center
                            glm::vec3(0.0f, 1.0f, 0.0f));  // Up vector

    auto proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

	m_lightCache.upload(device, cmd);

    const auto gpuSceneData = SceneRenderer::GPUSceneData{
		.view = view,
		.proj = proj,
		.viewProj = proj * view,
		.cameraPos = glm::vec4(0.f),
        .mousePos = {0.f, 0.f},
		.ambientColor = LinearColorNoAlpha::white(),
		.ambientIntensity = 0.2f,
		.lightsBuffer = m_lightCache.getBuffer().address,
        .numLights = (uint32_t)m_lightCache.getSize(),
		.materialsBuffer = renderer->getMaterialCache().getMaterialDataBufferAddress(),
    };
	sceneDataBuffer.uploadNewData(
		cmd, 
		device.getCurrentFrameIndex(), 
		(void *)&gpuSceneData,
		sizeof(SceneRenderer::GPUSceneData));
    renderer->getMaterialCache().upload(device, cmd);

    gfx::vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    gfx::vkutil::transitionImage(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 0.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    m_thumbnailGradientPass.draw(device, cmd, drawImage.getExtent2D());

    m_forwardRenderer.draw2(
        device,
        cmd,
		drawImage.getExtent2D(),
        sceneDataBuffer.getBuffer(),
        renderer->getMeshCache(),
        std::vector<MeshID>{renderer->getSphereMesh()},
        mat);

    vkCmdEndRendering(cmd);
}

void CustomThumbnail::generateModelThumbnail(gfx::CommandBuffer cmd, 
    std::vector<MeshID> mesh, 
    const fs::path &path) 
{
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();

    auto drawImage = device.getImage(m_thumbnails[path].first);
    auto depthImage = device.getImage(m_thumbnails[path].second);

    gfx::NBuffer sceneDataBuffer;
	sceneDataBuffer.init(
        device,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(SceneRenderer::GPUSceneData),
        gfx::FRAME_OVERLAP,
        "scene data");

    auto combinedMin = glm::vec3(std::numeric_limits<float>::max());
	auto combinedMax = glm::vec3(std::numeric_limits<float>::min());

    for (const auto &id : mesh)
	{
		const auto &m = renderer->getMesh(id);
		combinedMin = glm::min(combinedMin, m.boundingBox.min);
		combinedMax = glm::max(combinedMax, m.boundingBox.max);
	}

    auto center = (combinedMin + combinedMax) * 0.5f;
	auto size = combinedMax - combinedMin;
    float maxDim = glm::compMax(size);
    float radius = glm::length(size) * 0.5f;
    float distanceFromCenter = radius / std::tan(glm::radians(45.0f / 2.0f)); // Based on FOV
    glm::vec3 cameraPos = center + glm::vec3(0.0f, 0.0f, distanceFromCenter);

    auto view = glm::lookAt(cameraPos, center, glm::vec3(0.f, 1.f, 0.f));
    auto proj = glm::perspective(glm::radians(45.0f), 1.0f,
                                 distanceFromCenter * 0.1f,  // Near plane
                                 distanceFromCenter * 2.0f); // Far plane

	m_lightCache.upload(device, cmd);

    const auto gpuSceneData = SceneRenderer::GPUSceneData{
		.view = view,
		.proj = proj,
		.viewProj = proj * view,
		.cameraPos = glm::vec4(0.f),
        .mousePos = {0.f, 0.f},
		.ambientColor = LinearColorNoAlpha::white(),
		.ambientIntensity = 0.4f,
		.lightsBuffer = m_lightCache.getBuffer().address,
        .numLights = (uint32_t)m_lightCache.getSize(),
		.materialsBuffer = renderer->getMaterialCache().getMaterialDataBufferAddress(),
    };
	sceneDataBuffer.uploadNewData(
		cmd, 
		device.getCurrentFrameIndex(), 
		(void *)&gpuSceneData,
		sizeof(SceneRenderer::GPUSceneData));
    renderer->getMaterialCache().upload(device, cmd);

    gfx::vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    gfx::vkutil::transitionImage(cmd, depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = glm::vec4{1.f, 0.f, 0.f, 1.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    m_thumbnailGradientPass.draw(device, cmd, drawImage.getExtent2D());

    m_forwardRenderer.draw2(
        device,
        cmd,
		drawImage.getExtent2D(),
        sceneDataBuffer.getBuffer(),
        renderer->getMeshCache(),
        mesh,
        0,
        true);

    vkCmdEndRendering(cmd);

    auto width = m_size.x;
    auto height = m_size.y;

   auto outPath = ProjectManager::getConfig().getThumbnailCachePath() / (path.filename().string() + ".png");
  /* VK_CHECK(saveVkImageToPNG(device, device.getDevice(), device.getCommandPool(), device.getQueue(), drawImage.image, width,
                     height, outPath.string().c_str()));*/

}
} // namespace sky