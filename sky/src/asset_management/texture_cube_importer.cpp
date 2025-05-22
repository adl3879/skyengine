#include "texture_cube_importer.h"
#include "renderer/texture.h"
#include "core/project_management/project_manager.h"
#include "skypch.h"

#include <stb_image.h>

namespace sky 
{
Ref<TextureCube> TextureCubeImporter::importAsset(AssetHandle handle, AssetMetadata &metadata)
{
    const auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;
    return loadTexture(path);
}

Ref<TextureCube> TextureCubeImporter::loadTexture(const fs::path &path)
{
    auto data = CreateRef<TextureCube>();

    data->pixels = stbi_loadf(path.string().c_str(), 
        &data->width, 
        &data->height, 
        &data->channels, 
        STBI_rgb_alpha);
    if (!data->pixels)
    {
        SKY_CORE_ERROR("Failed to load image {}: {}", path.string(), stbi_failure_reason());
        return nullptr;
    }

    return data;
}
}