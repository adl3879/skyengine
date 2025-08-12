#include "scene_renderer.h"

#include <ImGuizmo.h>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan_core.h>

#include "IconsFontAwesome5.h"
#include "core/application.h"
#include "graphics/vulkan/vk_device.h"
#include "graphics/vulkan/vk_images.h"
#include "graphics/vulkan/vk_types.h"
#include "graphics/vulkan/vk_utils.h"
#include "imgui_impl_glfw.h"
#include "renderer/camera/camera.h"
#include "scene/components.h"
#include "asset_management/asset_manager.h"
#include "core/color.h"
#include "core/events/input.h"
#include "scene/entity.h"
#include "frustum_culling.h"
#include "core/helpers/image.h"
#include "scene/scene.h"
#include "scene/scene_manager.h"

namespace sky
{
static VkCommandBufferBeginInfo getCmdBufferBeginInfo();

SceneRenderer::SceneRenderer(gfx::Device &device)
	: m_device(device)
{
    m_meshDrawCommands.reserve(100);
}

SceneRenderer::~SceneRenderer() 
{
    m_forwardRenderer.cleanup(m_device);
    m_infiniteGridPass.cleanup(m_device);
    m_spriteRenderer.cleanup(m_device);
    m_depthResolvePass.cleanup(m_device);
    m_postFXPass.cleanup(m_device);
    m_ibl.cleanup(m_device);
    m_imguiBackend.cleanup(m_device);
}

bool SceneRenderer::isMultisamplingEnabled() const
{
    return gfx::vkutil::sampleCountToInt(m_samples) > 1;
}

void SceneRenderer::init(glm::ivec2 size) 
{
    createDrawImage(size);

    m_materialCache.init(m_device);
    initSceneData(); 

    initBuiltins();

    m_spriteRenderer.init(m_device, m_drawImageFormat);
    m_forwardRenderer.init(m_device, m_drawImageFormat, m_samples);
    m_infiniteGridPass.init(m_device, m_drawImageFormat, m_samples);
    m_depthResolvePass.init(m_device, m_depthImageFormat);
    m_postFXPass.init(m_device, m_drawImageFormat);
    m_ibl.init(m_device);

    ImGui::CreateContext();

    float fontSize = 26.f; 
    auto &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags = ImGuiConfigFlags_DockingEnable;
    io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", fontSize);

    float iconFontSize = fontSize * 2.0f / 3.0f;
    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF("res/fonts/" FONT_ICON_FILE_NAME_FAS, iconFontSize, &icons_config, icons_ranges);
    io.Fonts->AddFontFromFileTTF("res/fonts/" FONT_ICON_FILE_NAME_FAS, iconFontSize * 2, &icons_config, icons_ranges);

    m_imguiBackend.init(m_device, VK_FORMAT_B8G8R8A8_SRGB);
    ImGui_ImplGlfw_InitForVulkan(Application::getWindow()->getGLFWwindow(), true);
}

void SceneRenderer::initBuiltins() 
{
    {
		AssimpModelLoader loader("res/models/cube.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Cube] = addMeshToCache(mesh);
    }
	{
		AssimpModelLoader loader("res/models/plane.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Plane] = addMeshToCache(mesh);
    }
	{
		AssimpModelLoader loader("res/models/sphere.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Sphere] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/cylinder.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Cylinder] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/taurus.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Taurus] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/cone.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = m_materialCache.getDefaultMaterial();
		m_builtinModels[ModelType::Cone] = addMeshToCache(mesh);
	}
}

void SceneRenderer::initSceneData() 
{
    m_sceneDataBuffer.init(
        m_device,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(GPUSceneData),
        gfx::FRAME_OVERLAP,
        "scene data");
    m_gameDataBuffer.init(
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

ImageID SceneRenderer::createNewDrawImage(glm::ivec2 size, VkFormat format, VkSampleCountFlagBits samples) 
{
    const auto drawImageExtent = VkExtent3D{
        .width = (std::uint32_t)size.x,
        .height = (std::uint32_t)size.y,
        .depth = 1,
    };

	VkImageUsageFlags usages{};
	usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

	auto createImageInfo = gfx::vkutil::CreateImageInfo{
		.format = format,
		.usage = usages,
		.extent = drawImageExtent,
		.samples = samples,
	};
	// reuse the same id if creating again
	return m_device.createImage(createImageInfo);
}

ImageID SceneRenderer::createNewDepthImage(glm::ivec2 size, VkSampleCountFlagBits samples)
{
    const auto drawImageExtent = VkExtent3D{
        .width = (std::uint32_t)size.x,
        .height = (std::uint32_t)size.y,
        .depth = 1,
    };
    auto createInfo = gfx::vkutil::CreateImageInfo{
        .format = m_depthImageFormat,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .extent = drawImageExtent,
        .samples = samples,
    };
    // reuse the same id if creating again
    return m_device.createImage(createInfo);
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

        auto createInfo = gfx::vkutil::CreateImageInfo{
            .format = m_drawImageFormat,
            .usage = usages,
            .extent = drawImageExtent,
            .samples = m_samples,
        };

        m_sceneRenderTargets.color = m_device.createImage(createInfo);
        m_gameRenderTargets.color = m_device.createImage(createInfo);
        { // postFX
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            m_sceneRenderTargets.postFX = m_device.createImage(createInfo);
            m_gameRenderTargets.postFX = m_device.createImage(createInfo);
        }
    }

    { // setup resolve image
        VkImageUsageFlags usages{};
        usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

        auto info = gfx::vkutil::CreateImageInfo{
            .format = m_drawImageFormat,
            .usage = usages,
            .extent = drawImageExtent,
        };
        m_sceneRenderTargets.resolve = m_device.createImage(info);
        m_gameRenderTargets.resolve = m_device.createImage(info);
    }

    { // setup depth image
        auto createInfo = gfx::vkutil::CreateImageInfo{
            .format = m_depthImageFormat,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = drawImageExtent,
            .samples = m_samples,
        };
        m_sceneRenderTargets.depth = m_device.createImage(createInfo);
        m_gameRenderTargets.depth = m_device.createImage(createInfo);

        if (isMultisamplingEnabled()) 
        {
            createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            m_sceneRenderTargets.resolveDepth = m_device.createImage(createInfo);
            m_gameRenderTargets.resolveDepth = m_device.createImage(createInfo);
        }
    }
}

gfx::AllocatedImage SceneRenderer::getDrawImage() 
{
	return m_device.getImage(m_sceneRenderTargets.color);
}

void SceneRenderer::drawMesh(MeshID id, const glm::mat4 &transform, bool visibility, uint32_t uniqueId, MaterialID mat) 
{
    const auto &mesh = m_meshCache.getMesh(id);
    const auto worldBoundingSphere = edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    const auto dc = MeshDrawCommand{
        .meshId = id,
        .modelMatrix = transform,
        .isVisible = visibility,
        .uniqueId = uniqueId + 1,
        .worldBoundingSphere = worldBoundingSphere,
        .material = mat
    };

    //assert(m_meshDrawCommands.capacity() >= m_meshDrawCommands.size() + 1);
    m_meshDrawCommands.push_back(dc);
}

// Used for drag indicator, can be used to draw any model that can't be selected
void SceneRenderer::drawModel(Ref<Model> model, const glm::mat4 &transform) 
{
    for (const auto &mesh : model->meshes)
    {
        drawMesh(mesh, transform, true, -1, getMesh(mesh).material);
    }
}

void SceneRenderer::render(gfx::CommandBuffer &cmd, Ref<Scene> scene, RenderMode mode) 
{    
    auto camSystem = scene->getCameraSystem();
    Camera* cam = mode == RenderMode::Scene 
        ? static_cast<Camera*>(scene->getEditorCamera().get()) 
        : static_cast<Camera*>(camSystem->getActiveCameraForRendering());

    if (!cam) return;

    const auto &targets = mode == RenderMode::Scene ? m_sceneRenderTargets : m_gameRenderTargets;
    auto &sceneDataBuffer = mode == RenderMode::Scene ? m_sceneDataBuffer : m_gameDataBuffer;

    auto &lightCache = scene->getLightCache();
    {
        const auto gpuSceneData = GPUSceneData{
            .view = cam->getView(),
            .proj = cam->getProjection(),
            .viewProj = cam->getViewProjection(),
            .cameraPos = {cam->getPosition(), 1.f},
            .mousePos = scene->getViewportInfo().mousePos,
            .ambientColor = LinearColorNoAlpha::white(),
            .ambientIntensity = 0.3f,
            .irradianceMapId = m_ibl.getIrradianceMapId(),
            .prefilterMapId = m_ibl.getPrefilterMapId(),
            .brdfLutId = m_ibl.getBrdfLutId(),
            .lightsBuffer = lightCache.getBuffer().address,
            .numLights = (uint32_t)lightCache.getSize(),
            .materialsBuffer = m_materialCache.getMaterialDataBufferAddress(),
        };
        uint32_t bufferIndex = m_device.getCurrentFrameIndex();

        sceneDataBuffer.uploadNewData(
            cmd, 
            bufferIndex, 
            (void *)&gpuSceneData,
            sizeof(GPUSceneData));

        m_materialCache.upload(m_device, cmd);
        lightCache.upload(m_device, cmd);
	}

    const auto &drawImage = m_device.getImage(targets.color);
    const auto &resolveImage = m_device.getImage(targets.resolve);
    const auto &depthImage = m_device.getImage(targets.depth);
    const auto &postFXImage = m_device.getImage(targets.postFX);

    gfx::vkutil::transitionImage(cmd, 
        drawImage.image, 
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    gfx::vkutil::transitionImage(cmd, 
        depthImage.image, 
        VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    if (isMultisamplingEnabled())
    {
        gfx::vkutil::transitionImage(cmd, 
            resolveImage.image, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    m_ibl.draw(m_device, cmd, sceneDataBuffer.getBuffer());

    auto clearColor = mode == RenderMode::Scene
        ? glm::vec4{0.01f, 0.01f, 0.01f, 1.f}
        : camSystem->getActiveCameraForRendering()->getBackgroundColor();
    const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = clearColor,
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
        .resolveImageView = isMultisamplingEnabled() ? resolveImage.imageView : VK_NULL_HANDLE
    });

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
    {
        m_ibl.drawSky(m_device, cmd, drawImage.getExtent2D(), sceneDataBuffer.getBuffer());
        if (SceneManager::get().sceneIsType(SceneType::Scene3D)) {
            if (mode == RenderMode::Scene)
            {
                m_infiniteGridPass.draw(m_device, 
                    cmd, 
                    drawImage.getExtent2D(),
                    sceneDataBuffer.getBuffer());
            }
        }

		m_forwardRenderer.draw(m_device,
			cmd,
			drawImage.getExtent2D(),
			*cam,
			sceneDataBuffer.getBuffer(),
			m_meshCache,
			m_meshDrawCommands);

		m_spriteRenderer.flush(m_device, 
			cmd,
			drawImage.getExtent2D(), 
			sceneDataBuffer.getBuffer());

        if (mode == RenderMode::Scene) mousePicking(scene);
	}
    vkCmdEndRendering(cmd);
    
    gfx::vkutil::transitionImagesToShaderReadable(
        cmd,
        isMultisamplingEnabled() ? resolveImage.image : drawImage.image,
        depthImage.image);

    if (isMultisamplingEnabled())
    {
        const auto &resolveDepthImage = m_device.getImage(targets.resolveDepth);

        gfx::vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        const auto renderInfo = gfx::vkutil::createRenderingInfo({
            .renderExtent = resolveDepthImage.getExtent2D(),
            .depthImageView = resolveDepthImage.imageView,
        });

        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

        m_depthResolvePass.draw(m_device, cmd, targets.depth, gfx::vkutil::sampleCountToInt(m_samples));

        vkCmdEndRendering(cmd);

        // sync with post fx
        gfx::vkutil::transitionImage(
            cmd,
            resolveDepthImage.image,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    } 

    {
        ImageID sourceImageID = isMultisamplingEnabled() ? targets.resolve : targets.color;
        ImageID depthImageID = isMultisamplingEnabled() ? targets.resolveDepth : targets.depth;
 
        gfx::vkutil::transitionImage(
            cmd,
            postFXImage.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            
        const auto renderInfo = gfx::vkutil::createRenderingInfo({
            .renderExtent = postFXImage.getExtent2D(),
            .colorImageView = postFXImage.imageView,
        });
        
        vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);
        
        m_postFXPass.draw(
            m_device,
            cmd,
            sourceImageID,
            depthImageID,
            sceneDataBuffer.getBuffer());
            
        vkCmdEndRendering(cmd);
    } 

    gfx::vkutil::transitionImage(
        cmd,
        postFXImage.image,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SceneRenderer::renderImgui(gfx::CommandBuffer &cmd, 
    VkImage swapchainImage,
    uint32_t swapchainImageIndex)
{
    auto swapchain = m_device.getSwapchain();

    // Fences are reset here to prevent the deadlock in case swapchain becomes dirty
    swapchain.resetFences(m_device.getDevice(), m_device.getCurrentFrameIndex());

    auto swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    {
        // clear swapchain image
        VkImageSubresourceRange clearRange = gfx::vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        gfx::vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_GENERAL);
        swapchainLayout = VK_IMAGE_LAYOUT_GENERAL;

        const auto clearValue = VkClearColorValue{{1.f, 1.f, 1.f, 1.f}};
        vkCmdClearColorImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    }

    ZoneScopedN("Imgui draw");
    gfx::vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_imguiBackend.draw(cmd, 
        m_device, 
        swapchain.getImageView(swapchainImageIndex), 
        swapchain.getExtent(),
        ImGui::GetDrawData());
    
    gfx::vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void SceneRenderer::update(Ref<Scene> scene) 
{
    ZoneScopedN("Scene Renderer");

    {
		auto view = scene->getRegistry().view<TransformComponent, ModelComponent, VisibilityComponent>();
		for (auto &e : view)
		{
			auto [transform, modelComponent, visibility] = 
                view.get<TransformComponent, ModelComponent, VisibilityComponent>(e);

            if (modelComponent.type == ModelType::Custom)
            {
				AssetManager::getAssetAsync<Model>(modelComponent.handle, [=](const Ref<Model> &model){
					for (size_t i = 0; i < model->meshes.size(); i++) 
					{
                        const auto &mesh = model->meshes[i];
                        const auto material = modelComponent.customMaterialOverrides.contains(i) 
                            ? AssetManager::getAsset<MaterialAsset>(modelComponent.customMaterialOverrides.at(i))->material 
                            : getMesh(mesh).material;
                        
                        drawMesh(mesh, transform.getModelMatrix(), visibility,
                            static_cast<uint32_t>(e), material);
					}
				});
            }
            else
            {
                // draw builtin models
                const auto material = modelComponent.builtinMaterial != NULL_UUID ?
					AssetManager::getAsset<MaterialAsset>(modelComponent.builtinMaterial)->material :
                    m_materialCache.getDefaultMaterial();
                drawMesh(m_builtinModels[modelComponent.type], transform.getModelMatrix(), 
                    visibility, static_cast<uint32_t>(e), material);
            }
		}
    }
    {
        auto view = scene->getRegistry().view<TransformComponent, SpriteRendererComponent, VisibilityComponent>();
        for (auto &e : view)
        {
            auto [transform, spriteRenderer, visibility] =
                view.get<TransformComponent, SpriteRendererComponent, VisibilityComponent>(e);

            auto texture = AssetManager::getAsset<Texture2D>(spriteRenderer.textureHandle);
			m_spriteRenderer.drawSprite(m_device, {
				.position = {transform.getPosition().x - 0.5, transform.getPosition().y - 0.5},
				.size = {transform.getScale().x, transform.getScale().y},
				.color = spriteRenderer.tint,
				.rotation = transform.getRotation().z,
				.textureId = helper::loadImageFromTexture(texture, VK_FORMAT_R8G8B8A8_SRGB),
                .uniqueId = static_cast<uint32_t>(e) + 1,
			});
		}
    }

    updateLights(scene);
}

void SceneRenderer::updateMaterial(MaterialID id, Material material) 
{
    m_materialCache.updateMaterial(m_device, id, material);
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

void SceneRenderer::mousePicking(Ref<Scene> scene) 
{
    // TODO: mouse picking should only work on the viewport
    if (ImGuizmo::IsUsing() 
        || Input::isKeyPressed(Key::LeftAlt) 
        || !scene->getViewportInfo().isFocus) return;
    static bool called = false;

    if (Input::isMouseButtonPressed(Mouse::ButtonLeft))
    {
        called = true;
		auto storageBuffer = m_device.getStorageBuffer();

		void *mappedMemory = nullptr;
		VK_CHECK(vmaMapMemory(m_device.getAllocator(), storageBuffer.allocation, (void **)&mappedMemory));
		uint32_t *u = static_cast<uint32_t *>(mappedMemory);

		int selectedID = -1;
		for (int i = 0; i < gfx::DEPTH_ARRAY_SCALE; i++)
		{
            // Should be u[i]. this will only work in 2d
			if (u[i] != 0)
			{
				selectedID = u[i];
				auto ent = Entity{(entt::entity)(selectedID - 1), scene.get()};
				scene->setSelectedEntity(ent);
				break;
			}
		}

		std::memset(mappedMemory, 0, gfx::DEPTH_ARRAY_SCALE * sizeof(uint32_t));
		vmaUnmapMemory(m_device.getAllocator(), storageBuffer.allocation); // Unmap memory using VMA
    } else called = false;
}

void SceneRenderer::addTempModel(const fs::path &path, Ref<Model> model) 
{
    m_tempModels[path] = model;
}
} // namespace sky