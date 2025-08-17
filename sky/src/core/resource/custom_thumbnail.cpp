#include "custom_thumbnail.h"

#include "asset_management/asset.h"
#include "core/application.h"
#include "core/project_management/project_manager.h"
#include "core/helpers/image.h"
#include "core/uuid.h"
#include "graphics/vulkan/vk_images.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "graphics/vulkan/vk_types.h"
#include "renderer/camera/editor_camera.h"
#include "renderer/material.h"
#include "renderer/passes/forward_renderer.h"
#include "renderer/scene_renderer.h"
#include "scene/scene_manager.h"
#include "scene/components.h"

#include <filesystem>
#include <memory>
#include <stb_image_write.h>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace sky
{
CustomThumbnail::CustomThumbnail() 
{
    auto renderer = Application::getRenderer();
	auto &device = renderer->getDevice();

	m_lightCache.init(device);
	auto transform = Transform{};
	transform.rotateDegrees({120.f, 30.f, 0.f});
	m_lightCache.addLight(Light{
        .type = LightType::Directional, 
        .color = LinearColorNoAlpha::white(), 
        .intensity = 5.f}, transform);

    m_thumbnailGradientPass.init(device, m_drawImageFormat);
	m_forwardRenderer.init(device, m_drawImageFormat, VK_SAMPLE_COUNT_1_BIT);
    m_formatConverterPass.init(device, m_drawImageFormat);
    m_infiniteGridPass.init(device, m_drawImageFormat, VK_SAMPLE_COUNT_1_BIT);
    m_spriteRenderer.init(device, m_drawImageFormat);

    // Material preview
    auto drawImage = renderer->createNewDrawImage(m_size, m_drawImageFormat);
    auto depthImage = renderer->createNewDepthImage(m_size);
    m_matPreviewImage = std::make_pair(drawImage, depthImage);
}

CustomThumbnail::~CustomThumbnail()
{
    auto &device = Application::getRenderer()->getDevice();
    m_thumbnailGradientPass.cleanup(device);
    m_forwardRenderer.cleanup(device);
    m_formatConverterPass.cleanup(device);
    m_infiniteGridPass.cleanup(device);
    m_spriteRenderer.cleanup(device);
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
            AssetManager::getAssetAsync<MaterialAsset>(handle, [=, this](const Ref<MaterialAsset> &materialAsset) {
                if (materialAsset) {
                    m_readyMaterials.push_back({path, materialAsset->material});
                }
            });
            break;
        }
        case AssetType::Mesh:
        {
            auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Mesh);
            AssetManager::getAssetAsync<Model>(handle, [=, this](const Ref<Model> &model) {
                if (model && !model->meshes.empty()) {
                    m_readyModels.push_back({path, model->meshes});
                }
            });
            break;
        }
        case AssetType::Texture2D:
        {
            auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Texture2D);
            AssetManager::getAssetAsync<Texture2D>(handle, [=, this](const Ref<Texture2D> &texture) {
                if (texture) {
                    auto img = helper::loadImageFromTexture(texture, VK_FORMAT_R8G8B8A8_UNORM);
                    m_readyTextures.push_back({path, img});
                }
            });
            break;
        }
        default: break;
        }

        m_queue.pop();
        break;
    }
    
    // Process thumbnails based on current state
    switch (m_currentProcessingState)
    {
    case ThumbnailProcessingState::None:
        // Determine next state based on what's available
        if (!m_readyMaterials.empty()) m_currentProcessingState = ThumbnailProcessingState::Material;
        else if (!m_readyModels.empty()) m_currentProcessingState = ThumbnailProcessingState::Model;
        else if (!m_readyScene.empty()) m_currentProcessingState = ThumbnailProcessingState::Scene;
        else if (!m_readyTextures.empty()) m_currentProcessingState = ThumbnailProcessingState::Texture;
        break;
        
    case ThumbnailProcessingState::Material:
        if (!m_readyMaterials.empty()) {
            auto& readyMaterial = m_readyMaterials.front();
            auto thumbnail = m_thumbnails[readyMaterial.path];
            gfx::NBuffer sceneDataBuffer;
            generateMaterialThumbnail(cmd, 
                readyMaterial.materialId, 
                thumbnail);
            saveThumbnailToFile(cmd, thumbnail.first, readyMaterial.path);
            m_readyMaterials.pop_front();
        }
        m_currentProcessingState = ThumbnailProcessingState::None;
        break;
        
    case ThumbnailProcessingState::Model:
        if (!m_readyModels.empty()) {
            auto& readyModel = m_readyModels.front();
            generateModelThumbnail(cmd, readyModel.meshes, readyModel.path);
            m_readyModels.pop_front();
        }
        m_currentProcessingState = ThumbnailProcessingState::None;
        break;
        
    case ThumbnailProcessingState::Scene:
        if (!m_readyScene.empty()) {
            auto &readyScene = m_readyScene.front();
            generateSceneThumbnail(cmd, readyScene.path, readyScene.materialId);
            m_readyScene.pop_front();
        }
        m_currentProcessingState = ThumbnailProcessingState::None;
        break;
        
    case ThumbnailProcessingState::Texture:
        if (!m_readyTextures.empty()) {
            auto &readyTexture = m_readyTextures.front();
            generateTextureThumbnail(cmd, readyTexture.textureId, readyTexture.path);
            m_readyTextures.pop_front();
        }
        m_currentProcessingState = ThumbnailProcessingState::None;
        break;
    }

    renderMaterialPreview(cmd);
}

ImageID CustomThumbnail::getOrCreateThumbnail(const fs::path &path) 
{
	if (m_thumbnails.contains(path)) return m_thumbnails.at(path).first;

	// Check if we have a cached thumbnail file
	std::string assetPathHash = std::to_string(std::hash<std::string>{}(path.string()));
	fs::path cacheFilePath = ProjectManager::getConfig().getThumbnailCachePath() / (assetPathHash + ".png");
	if (fs::exists(cacheFilePath))
    {
        m_thumbnails[path].first = helper::loadImageFromFile(cacheFilePath);
        return m_thumbnails[path].first;
	}

    auto assetType = getAssetTypeFromFileExtension(path.extension());
    if (assetType != AssetType::Scene
        || assetType != AssetType::TextureCube)
    {
        auto renderer = Application::getRenderer();
        auto drawImage = renderer->createNewDrawImage(m_size, m_drawImageFormat);
        auto depthImage = renderer->createNewDepthImage(m_size);
        m_thumbnails[path] = std::make_pair(drawImage, depthImage);
        m_queue.push(path);
        return drawImage;
    }

    return NULL_IMAGE_ID;
}

void CustomThumbnail::refreshThumbnail(const fs::path &path)
{
    fs::path canonicalPath = fs::weakly_canonical(path); 
    if (m_thumbnails.contains(canonicalPath)) m_thumbnails.erase(canonicalPath);
    std::string assetPathHash = std::to_string(std::hash<std::string>{}(canonicalPath.string()));
	fs::path cacheFilePath = ProjectManager::getConfig().getThumbnailCachePath() / (assetPathHash + ".png");
    if (fs::exists(cacheFilePath)) 
        fs::remove(cacheFilePath);
}

void CustomThumbnail::refreshSceneThumbnail(const fs::path &path)
{
    auto renderer = Application::getRenderer();
    m_readyScene.push_back({path, renderer->getSceneImage()});
}

void CustomThumbnail::generateSceneThumbnail(gfx::CommandBuffer cmd, const fs::path &path, ImageID imageId)
{
    auto scene = SceneManager::get().getEditorScene();
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();

    if (m_thumbnails[path].first != NULL_IMAGE_ID)
    {
        auto drawImage = renderer->createNewDrawImage(m_size, m_drawImageFormat);
        auto depthImage = renderer->createNewDepthImage(m_size);
        m_thumbnails[path] = std::make_pair(drawImage, depthImage);
    }

    auto drawImage = device.getImage(m_thumbnails[path].first);
    auto depthImage = device.getImage(m_thumbnails[path].second);

    gfx::NBuffer sceneDataBuffer;
	sceneDataBuffer.init(
        device,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(SceneRenderer::GPUSceneData),
        gfx::FRAME_OVERLAP,
        "scene data");

    auto cam = std::make_unique<EditorCamera>(45.f, 16 / 9, 0.1f, 1000.f);
    cam->setViewportSize(m_size);
    // 0.01f is just a random number (not significant for any reason)
    cam->update(0.01f);

    auto lightCache = scene->getLightCache();

    const auto gpuSceneData = SceneRenderer::GPUSceneData{
		.view = cam->getView(),
		.proj = cam->getProjection(),
		.viewProj = cam->getViewProjection(),
		.cameraPos = glm::vec4(0.f),
        .mousePos = {0.f, 0.f},
		.ambientColor = LinearColorNoAlpha::white(),
		.ambientIntensity = 0.4f,
        .irradianceMapId  = renderer->getIBL().getIrradianceMapId(),
        .prefilterMapId  = renderer->getIBL().getPrefilterMapId(),
        .brdfLutId  = renderer->getIBL().getBrdfLutId(),
		.lightsBuffer = lightCache.getBuffer().address,
        .numLights = (uint32_t)lightCache.getSize(),
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
        .colorImageClearValue = glm::vec4{0.01f, 0.01f, 0.01f, 1.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
    });

    {
        auto view = scene->getRegistry().view<TransformComponent, SpriteRendererComponent, VisibilityComponent>();
        for (auto &e : view)
        {
            auto [transform, spriteRenderer, visibility] =
                view.get<TransformComponent, SpriteRendererComponent, VisibilityComponent>(e);

            auto texture = AssetManager::getAsset<Texture2D>(spriteRenderer.textureHandle);
			m_spriteRenderer.drawSprite(device, {
				.position = {transform.getPosition().x - 0.5, transform.getPosition().y - 0.5},
				.size = {transform.getScale().x, transform.getScale().y},
				.color = spriteRenderer.tint,
				.rotation = transform.getRotation().z,
				.textureId = helper::loadImageFromTexture(texture, VK_FORMAT_R8G8B8A8_SRGB),
                .uniqueId = static_cast<uint32_t>(e) + 1,
			});
		}
    }

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    if (SceneManager::get().sceneIsType(SceneType::Scene3D)) {
        renderer->getIBL().drawSky(device, cmd, drawImage.getExtent2D(), sceneDataBuffer.getBuffer());
        m_infiniteGridPass.draw(device, 
            cmd, 
            drawImage.getExtent2D(),
            sceneDataBuffer.getBuffer());
    }

    m_forwardRenderer.draw3(device, 
        cmd, 
        drawImage.getExtent2D(), 
        *scene->getEditorCamera(), 
        sceneDataBuffer.getBuffer(),
        renderer->getBuiltInModels(),
        renderer->getMeshCache(),
        renderer->getMaterialCache(),
        scene); 

    m_spriteRenderer.flush(device, cmd, drawImage.getExtent2D(), sceneDataBuffer.getBuffer());

    vkCmdEndRendering(cmd);

    saveThumbnailToFile(cmd, m_thumbnails[path].first, path);
}

void CustomThumbnail::generateMaterialThumbnail(gfx::CommandBuffer cmd, 
    MaterialID mat, 
    std::pair<ImageID, ImageID> thumbnail) 
{
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();

    auto drawImage = device.getImage(thumbnail.first);
    auto depthImage = device.getImage(thumbnail.second);

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

void CustomThumbnail::renderMaterialPreview(gfx::CommandBuffer cmd)
{
    if (m_matPreviewAssetHandle != NULL_UUID)
    {
        auto mat = AssetManager::getAsset<MaterialAsset>(m_matPreviewAssetHandle);
        generateMaterialThumbnail(cmd, mat->material, m_matPreviewImage);
    }
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

    saveThumbnailToFile(cmd, m_thumbnails[path].first, path);
}

void CustomThumbnail::generateTextureThumbnail(gfx::CommandBuffer cmd, ImageID imageId, const fs::path &path)
{
    auto renderer = Application::getRenderer();
    auto &device = Application::getRenderer()->getDevice();

    auto targetImage = device.getImage(m_thumbnails[path].first);

    gfx::vkutil::transitionImage(cmd, targetImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = targetImage.getExtent2D(),
        .colorImageView = targetImage.imageView,
        .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 1.f},
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    m_formatConverterPass.draw(
        device,
        cmd,
        imageId,
		targetImage.getExtent2D());

    vkCmdEndRendering(cmd);

    saveThumbnailToFile(cmd, m_thumbnails[path].first, path);
}

void CustomThumbnail::saveThumbnailToFile(gfx::CommandBuffer cmd, ImageID imageId, const fs::path &path)
{
    auto &device = Application::getRenderer()->getDevice();
    auto image = device.getImage(imageId);
    
    // Create a staging buffer to download the image data
    VkDeviceSize imageSize = m_size.x * m_size.y * 4; // RGBA8 format (4 bytes per pixel)
    
    gfx::AllocatedBuffer stagingBuffer;
    stagingBuffer = device.createBuffer(imageSize, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VMA_MEMORY_USAGE_GPU_TO_CPU);
    
    // Transition image layout for transfer
    gfx::vkutil::transitionImage(cmd, image.image, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    
    // Copy image to buffer
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y), 1};
    
    vkCmdCopyImageToBuffer(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        stagingBuffer.buffer, 1, &region);
    
    // Add a buffer memory barrier to ensure the copy is complete before reading
    VkBufferMemoryBarrier bufferBarrier{};
    bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    bufferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    bufferBarrier.buffer = stagingBuffer.buffer;
    bufferBarrier.offset = 0;
    bufferBarrier.size = imageSize;
    
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        0, nullptr,
        1, &bufferBarrier,
        0, nullptr
    );
    
    // Transition back to original layout
    gfx::vkutil::transitionImage(cmd, image.image, 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // --- Submit and wait for GPU to finish ---
    auto rawCmd = cmd.handle;

    // End the command buffer if not already done
    vkEndCommandBuffer(rawCmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rawCmd;

    // Create fence
    VkFence fence;
    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &fence);

    // Submit and wait
    vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, fence);
    vkWaitForFences(device.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device.getDevice(), fence, nullptr);
    
    // Map memory and save to file
    void* data;
    vmaMapMemory(device.getAllocator(), stagingBuffer.allocation, &data);
    
    // Create parent directories if they don't exist
    fs::path cacheDir = ProjectManager::getConfig().getThumbnailCachePath();
    if (!fs::exists(cacheDir)) 
        fs::create_directories(cacheDir);
    
    // Generate a cache filename based on the asset path
    std::string assetPathHash = std::to_string(std::hash<std::string>{}(path.string()));
    fs::path cacheFilePath = ProjectManager::getConfig().getThumbnailCachePath() / (assetPathHash + ".png");
    
    // Save image data to PNG file using stb_image_write
    stbi_flip_vertically_on_write(true);
    stbi_write_png(cacheFilePath.string().c_str(), m_size.x, m_size.y, 4, data, m_size.x * 4);
    
    vmaUnmapMemory(device.getAllocator(), stagingBuffer.allocation);
    
    // Clean up the staging buffer
    vmaDestroyBuffer(device.getAllocator(), stagingBuffer.buffer, stagingBuffer.allocation);
}
} // namespace sky