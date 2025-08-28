#pragma once

#include <skypch.h>
#include <utility>

#include "asset_management/asset.h"
#include "core/filesystem.h"
#include "core/uuid.h"
#include "graphics/vulkan/vk_device.h"
#include "graphics/vulkan/vk_types.h"
#include "renderer/light_cache.h"
#include "renderer/passes/infinite_grid.h"
#include "renderer/passes/thumbnail_gradient.h"
#include "renderer/passes/forward_renderer.h"
#include "renderer/passes/format_converter.h"
#include "renderer/sprite_renderer.h"

namespace sky
{
class CustomThumbnail
{
  public:
    CustomThumbnail();
	~CustomThumbnail();

	static CustomThumbnail &get() 
	{
		static CustomThumbnail instance;
		return instance;
	}

	ImageID getOrCreateThumbnail(const fs::path &path);
	void render(gfx::CommandBuffer cmd);
	void refreshThumbnail(const fs::path &path);
    void refreshSceneThumbnail(const fs::path &path);

    void setMaterialPreviewAssetHandle(AssetHandle handle) { m_matPreviewAssetHandle = handle; }
    auto getMaterialPreview() { return m_matPreviewImage.first; }

  private:
	void generateMaterialThumbnail(gfx::CommandBuffer cmd, MaterialID mat, std::pair<ImageID, ImageID> thumbnail);
	void generateModelThumbnail(gfx::CommandBuffer cmd, std::vector<MeshID> mesh, const fs::path &path);
    void generateSceneThumbnail(gfx::CommandBuffer cmd, const fs::path &path, ImageID image);
    void generateTextureThumbnail(gfx::CommandBuffer cmd, ImageID image, const fs::path &path);

    void saveThumbnailToFile(gfx::CommandBuffer cmd, ImageID imageId, const fs::path &path);
    void renderMaterialPreview(gfx::CommandBuffer cmd);

  private:
	std::map<fs::path, std::pair<ImageID, ImageID>> m_thumbnails;
    std::queue<std::filesystem::path> m_queue;

	const glm::ivec2 m_size = {256, 256};
    LightCache m_lightCache;
    std::vector<std::pair<Light, Transform>> m_lights;
	VkFormat m_drawImageFormat{VK_FORMAT_R8G8B8A8_UNORM};

	ThumbnailGradientPass m_thumbnailGradientPass;
    FormatConverterPass m_formatConverterPass;
    ForwardRendererPass m_forwardRenderer;
    InfiniteGridPass m_infiniteGridPass;
    SpriteBatchRenderer m_spriteRenderer;

    // Mat preview
    std::pair<ImageID, ImageID> m_matPreviewImage;
    AssetHandle m_matPreviewAssetHandle = NULL_UUID;

    struct ReadyModel {
        fs::path path;
        std::vector<MeshID> meshes;
    };
    
    struct ReadyMaterial {
        fs::path path;
        MaterialID materialId;
    };

    struct ReadyScene {
        fs::path path;
        ImageID sceneId;
    };

    struct ReadyTexture {
        fs::path path;
        ImageID textureId;
    };
    
    std::deque<ReadyModel> m_readyModels;
    std::deque<ReadyMaterial> m_readyMaterials;
    std::deque<ReadyMaterial> m_readyScene;
    std::deque<ReadyTexture> m_readyTextures;

    enum class ThumbnailProcessingState 
    {
        None,
        Material,
        Model,
        Scene,
        Texture
    };
    ThumbnailProcessingState m_currentProcessingState = ThumbnailProcessingState::None;
};
} // namespace sky