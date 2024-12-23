#include "scene_renderer.h"

#include <ImGuizmo.h>
#include <tracy/Tracy.hpp>

#include "core/application.h"
#include "graphics/vulkan/vk_images.h"
#include "asset_management/texture_importer.h"
#include "scene/components.h"
#include "asset_management/asset_manager.h"
#include "core/color.h"
#include "core/events/input.h"
#include "scene/entity.h"
#include "frustum_culling.h"

namespace sky
{
SceneRenderer::SceneRenderer(gfx::Device &device)
	: m_device(device)
{
    m_meshDrawCommands.reserve(100);
}

SceneRenderer::~SceneRenderer() 
{
    m_forwardRenderer.cleanup(m_device);
    m_infiniteGridPass.cleanup(m_device);
}

void SceneRenderer::init(glm::ivec2 size) 
{
    createDrawImage(size);

    m_materialCache.init(m_device);
    initSceneData(); 

    m_forwardRenderer.init(m_device);
    m_infiniteGridPass.init(m_device);

    initBuiltins();
}

void SceneRenderer::initBuiltins() 
{
    {
		AssimpModelLoader loader("res/models/cube.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Cube] = addMeshToCache(mesh);
    }
	{
		AssimpModelLoader loader("res/models/plane.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Plane] = addMeshToCache(mesh);
    }
	{
		AssimpModelLoader loader("res/models/sphere.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Sphere] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/cylinder.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Cylinder] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/taurus.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Taurus] = addMeshToCache(mesh);
	}
	{
		AssimpModelLoader loader("res/models/cone.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Cone] = addMeshToCache(mesh);
	}
	{
		/*AssimpModelLoader loader("res/models/capsule.glb");
		auto mesh = loader.getMeshes()[0].mesh;
		mesh.material = addMaterialToCache(Material{});
		m_builtinModels[ModelType::Capsule] = addMeshToCache(mesh);*/
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

void SceneRenderer::drawMesh(MeshID id, const glm::mat4 &transform, bool visibility, uint32_t uniqueId) 
{
    const auto &mesh = m_meshCache.getMesh(id);
    const auto worldBoundingSphere = edge::calculateBoundingSphereWorld(transform, mesh.boundingSphere, false);

    const auto dc = MeshDrawCommand{
        .meshId = id,
        .modelMatrix = transform,
        .isVisible = visibility,
        .uniqueId = uniqueId + 1,
        .worldBoundingSphere = worldBoundingSphere,
    };

    //assert(m_meshDrawCommands.capacity() >= m_meshDrawCommands.size() + 1);
    m_meshDrawCommands.push_back(dc);
}

void SceneRenderer::drawModel(Ref<Model> model, const glm::mat4 &transform) 
{
    for (const auto &mesh : model->meshes)
    {
        drawMesh(mesh, transform, true, -1);
    }
}

void SceneRenderer::render(gfx::CommandBuffer cmd, Ref<Scene> scene) 
{
    ZoneScopedN("Scene Renderer");

    auto drawImage = m_device.getImage(m_drawImageID);
    auto depthImage = m_device.getImage(m_depthImageID);

    {
		auto view = scene->getRegistry().view<TransformComponent, ModelComponent, VisibilityComponent>();
		for (auto &e : view)
		{
			auto [transform, model, visibility] = view.get<TransformComponent, ModelComponent, VisibilityComponent>(e);

            if (model.type == ModelType::Custom)
            {
				AssetManager::getAssetAsync<Model>(model.handle, [=](const Ref<Model> &model){
					for (const auto &mesh : model->meshes) 
					{
                        drawMesh(mesh, transform.getModelMatrix(), visibility,
                            static_cast<uint32_t>(e));
					}
				});
            }
            else
            {
                drawMesh(m_builtinModels[model.type], transform.getModelMatrix(), 
                    visibility, static_cast<uint32_t>(e));
            }
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
            .mousePos = scene->getViewportInfo().mousePos,
            .viewportSize = scene->getViewportInfo().size,
            .ambientColor = LinearColorNoAlpha::white(),
            .ambientIntensity = 0.1f,
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

        m_materialCache.upload(m_device, cmd);
        lightCache.upload(m_device, cmd);
	}

    const auto renderInfo = gfx::vkutil::createRenderingInfo({
        .renderExtent = drawImage.getExtent2D(),
        .colorImageView = drawImage.imageView,
        .colorImageClearValue = glm::vec4{0.f, 0.f, 0.f, 1.f},
        .depthImageView = depthImage.imageView,
        .depthImageClearValue = 1.f,
    });

    gfx::vkutil::transitionImage(cmd, 
        drawImage.image, 
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(cmd, &renderInfo.renderingInfo);

    m_infiniteGridPass.draw(m_device, 
        cmd, 
        drawImage.getExtent2D(),
        m_sceneDataBuffer.getBuffer());

    m_forwardRenderer.draw(m_device,
        cmd,
        drawImage.getExtent2D(),
        camera,
        m_sceneDataBuffer.getBuffer(),
        m_meshCache,
        m_meshDrawCommands);

    mousePicking(scene);
    vkCmdEndRendering(cmd);

    clearDrawCommands();
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
    if (ImGuizmo::IsUsing() || Input::isKeyPressed(Key::LeftAlt) || !scene->getViewportInfo().isFocus) return;
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