#pragma once

#include <skypch.h>

#include "renderer/scene_renderer.h"
#include "renderer/passes/thumbnail_gradient.h"

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

	const glm::ivec2 m_size = {256, 256};
    LightCache m_lightCache;
	ThumbnailGradientPass m_thumbnailGradientPass;
};
} // namespace sky