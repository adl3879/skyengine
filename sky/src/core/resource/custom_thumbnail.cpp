#include "custom_thumbnail.h"

#include "core/application.h"
#include "graphics/vulkan/vk_images.h"
#include "asset_management/asset_manager.h"
#include "asset_management/editor_asset_manager.h"
#include "renderer/passes/forward_renderer.h"
#include "scene/scene_manager.h"

#include "renderer/camera/camera.h"

namespace sky
{
CustomThumbnail::CustomThumbnail() 
{
	auto &device = Application::getRenderer()->getDevice();

	m_lightCache.init(device);
	auto transform = Transform{};
	transform.rotateDegrees({120.f, 30.f, 0.f});
	m_lightCache.addLight(Light{
        .type = LightType::Directional, 
        .color = LinearColorNoAlpha::white(), 
        .intensity = 3.f}, transform);

    m_thumbnailGradientPass.init(device);
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

	auto drawImage = Application::getRenderer()->createNewDrawImage(m_size);
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
		.ambientIntensity = 0.05f,
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

    renderer->getForwardRendererPass().draw2(
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
		.ambientIntensity = 0.05f,
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

    renderer->getForwardRendererPass().draw2(
        device,
        cmd,
		drawImage.getExtent2D(),
        sceneDataBuffer.getBuffer(),
        renderer->getMeshCache(),
        mesh,
        0,
        true);

    vkCmdEndRendering(cmd);
}
} // namespace sky