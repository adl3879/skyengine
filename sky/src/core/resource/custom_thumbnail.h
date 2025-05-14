#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_device.h"
#include "renderer/scene_renderer.h"
#include "renderer/passes/thumbnail_gradient.h"
#include "renderer/passes/forward_renderer.h"

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

	void render(gfx::CommandBuffer cmd);
	ImageID getOrCreateThumbnail(const fs::path &path);
	void refreshThumbnail(const fs::path &path);

  private:
	void generateMaterialThumbnail(gfx::CommandBuffer cmd, MaterialID mat, const fs::path &path);
	void generateModelThumbnail(gfx::CommandBuffer cmd, std::vector<MeshID> mesh, const fs::path &path);

  private:
	std::map<fs::path, std::pair<ImageID, ImageID>> m_thumbnails;
    std::queue<std::filesystem::path> m_queue;
    gfx::CommandBuffer cmdBuffer;

	const glm::ivec2 m_size = {256, 256};
    LightCache m_lightCache;
	ThumbnailGradientPass m_thumbnailGradientPass;
    ForwardRendererPass m_forwardRenderer;
	VkFormat m_drawImageFormat{VK_FORMAT_R8G8B8A8_UNORM};
    
    struct ReadyModel {
        fs::path path;
        std::vector<MeshID> meshes;
    };
    
    struct ReadyMaterial {
        fs::path path;
        MaterialID materialId;
    };
    
    std::deque<ReadyModel> m_readyModels;
    std::deque<ReadyMaterial> m_readyMaterials;
    
    struct ThumbnailRenderRequest {
        fs::path path;
        AssetHandle handle;
    };
    std::vector<std::shared_ptr<ThumbnailRenderRequest>> m_pendingRequests;
};
} // namespace sky