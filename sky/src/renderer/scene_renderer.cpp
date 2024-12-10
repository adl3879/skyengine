#include "scene_renderer.h"

#include "core/application.h"
#include "graphics/vulkan/vk_images.h"
#include "asset_management/texture_importer.h"
#include "scene/components.h"
#include "asset_management/asset_manager.h"
#include "core/color.h"

namespace sky
{
SceneRenderer::SceneRenderer(gfx::Device &device)
	: m_device(device)
{
}

SceneRenderer::~SceneRenderer() {}

void SceneRenderer::init(glm::ivec2 size) 
{
    createDrawImage(size);

    m_materialCache.init(m_device);
    initSceneData();

    m_forwardRenderer.init(m_device);
}

void SceneRenderer::initSceneData() 
{
    m_sceneDataBuffer.init(
        m_device,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(GPUSceneData),
        gfx::FRAME_OVERLAP,
        "scene data");
}

MeshID SceneRenderer::addMeshToCache(const Mesh& mesh)
{
    return m_meshCache.addMesh(m_device, mesh);
}

MaterialID SceneRenderer::addMaterialToCache(const Material &material)
{
    return m_materialCache.addMaterial(m_device, material);
}

ImageID SceneRenderer::createImage(const gfx::vkutil::CreateImageInfo& createInfo, void* pixelData)
{
    return m_device.createImage(createInfo, pixelData);
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

void SceneRenderer::render(gfx::CommandBuffer cmd, Ref<Scene> scene) 
{
    auto drawImage = m_device.getImage(m_drawImageID);
    auto depthImage = m_device.getImage(m_depthImageID);

    {
		auto view = scene->getRegistry().view<TransformComponent, ModelComponent, VisibilityComponent>();
		for (auto &e : view)
		{
			auto [transform, model, visibility] = view.get<TransformComponent, ModelComponent, VisibilityComponent>(e);
			
			AssetManager::getAssetAsync<Model>(model.handle, [=](const Ref<Model> &model){
				for (const auto &mesh : model->meshes) 
				{
					addDrawCommand({
						.meshId = mesh,
						.modelMatrix = transform.getModelMatrix(),
						.isVisible = visibility,
					});
				}
			});
		}
    }
    updateLights(scene);

    auto &camera = scene->getCamera();
    auto &lightCache = scene->getLightCache();
    {
        const auto gpuSceneData = GPUSceneData{
            .view = camera.getView(),
            .proj = camera.getProjection(),
            .viewProj = camera.getViewProjection(),
            .cameraPos = camera.getPosition(),
            .ambientColor = LinearColorNoAlpha::white(),
            .ambientIntensity = 0.0f,
            .lightsBuffer = lightCache.getBuffer().address,
            .numLights = (uint32_t)lightCache.getSize(),
            .sunlightIndex = lightCache.getSunlightIndex(), 
            .materialsBuffer = m_materialCache.getMaterialDataBufferAddress(),
        };
        m_sceneDataBuffer.uploadNewData(
            cmd, 
            m_device.getCurrentFrameIndex(), 
            (void *)&gpuSceneData,
            sizeof(GPUSceneData));

        lightCache.upload(m_device, cmd);
	}

    const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 1.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 0.f,
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
        camera,
        m_sceneDataBuffer.getBuffer(),
        m_meshCache,
        m_meshDrawCommands);

    vkCmdEndRendering(cmd);

    m_meshDrawCommands.clear();
}

void SceneRenderer::updateLights(Ref<Scene> scene) 
{
    auto &lightCache = scene->getLightCache();
	{
        auto view = scene->getRegistry().view<TransformComponent, DirectionalLightComponent>();
		for (auto &e : view)
		{
			auto [transform, dl] = view.get<TransformComponent, DirectionalLightComponent>(e);
            lightCache.updateLight(dl.light.id, dl.light, transform);
		}
    }
	{
        auto view = scene->getRegistry().view<TransformComponent, PointLightComponent>();
		for (auto &e : view)
		{
			auto [transform, pl] = view.get<TransformComponent, PointLightComponent>(e);
            lightCache.updateLight(pl.light.id, pl.light, transform);
		}
    }
	{
        auto view = scene->getRegistry().view<TransformComponent, SpotLightComponent>();
		for (auto &e : view)
		{
			auto [transform, sl] = view.get<TransformComponent, SpotLightComponent>(e);
            lightCache.updateLight(sl.light.id, sl.light, transform);
		}
    }
}
} // namespace sky