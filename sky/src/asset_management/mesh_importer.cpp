#include "mesh_importer.h"

#include "renderer/model_loader.h"
#include "core/project_management/project_manager.h"
#include "asset_manager.h"
#include "renderer/texture.h"
#include "core/application.h"

namespace sky
{
static ImageID createMaterialImage(Ref<Texture2D> tex, VkFormat format, VkImageUsageFlags usage, bool mipMap) 
{
	auto renderer = Application::getRenderer();
    return renderer->createImage(
        {
            .format = format,
            .usage = usage |      //
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | // for uploading pixel data to image
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,  // for generating mips
            .extent =
                VkExtent3D{
                    .width = (std::uint32_t)tex->width,
                    .height = (std::uint32_t)tex->height,
                    .depth = 1,
                },
            .mipMap = mipMap,
        },
        tex->pixels);
}

Ref<Model> MeshImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
{
    const auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;
	return loadModel(handle, path); 
}

Ref<Model> MeshImporter::loadModel(AssetHandle handle, const fs::path &path) 
{
	AssimpModelLoader modelLoader(path);
    std::vector<Mesh> meshes;
	auto renderer = Application::getRenderer();

	for (auto &mesh : modelLoader.getMeshes())
	{
		Material material;

		// load textures
		auto materialPaths = mesh.materialPaths;
		if (materialPaths.albedoTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.albedoTexture);

			// add texture as a dependency
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(handle);
			material.albedoTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		if (materialPaths.normalMapTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.normalMapTexture);
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
			material.normalMapTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		if (materialPaths.metallicsTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.metallicsTexture);
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
			material.metallicTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		if (materialPaths.roughnessTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.roughnessTexture);
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
			material.roughnessTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		if (materialPaths.ambientOcclusionTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.ambientOcclusionTexture);
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
			material.ambientOcclusionTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		if (materialPaths.emissiveTexture != "")
		{
			auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.emissiveTexture);
			AssetManager::getMetadata(handle).dependencies.push_back(textureHandle);

			auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
			material.emissiveTexture =
				createMaterialImage(tex, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
		}

		// create add material id to mesh
		mesh.mesh.materialID = renderer->addMaterialToCache(material);
		meshes.push_back(mesh.mesh);
	}

	return CreateRef<Model>(meshes);
}
} // namespace sky