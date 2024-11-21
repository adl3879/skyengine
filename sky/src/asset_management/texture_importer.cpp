#include "texture_importer.h"

#include "core/project_management/project_manager.h"
#include <stb_image.h>

namespace sky
{
Ref<Texture2D> TextureImporter::importAsset(AssetHandle handle, AssetMetadata &metadata)
{
    const auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;
    return loadTexture(path);
}

Ref<Texture2D> TextureImporter::loadTexture(const fs::path &path) 
{
    auto data = CreateRef<Texture2D>();
    data->shouldSTBFree = true;

    stbi_set_flip_vertically_on_load(1);
	data->pixels = stbi_load(path.string().c_str(), &data->width, &data->height, &data->channels, 4);
	if (!data->pixels)
    {
        SKY_CORE_ERROR("Failed to load image {}: {}", path.string(), stbi_failure_reason());
        return {};
    }

    data->channels = 4;
    return data;
}
} // namespace sky