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
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    Buffer data;

    {
        std::string pathStr = path.string();
        data.data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
        channels = 4;
    }

    if (data.data == nullptr)
    {
        SKY_CORE_ERROR("TextureImporter::ImportTexture2D - Could not load texture from filepath: {}", path.string());
        return nullptr;
    }

    // TODO: think about this
    data.size = width * height * channels;

    TextureSpecification spec;
    spec.width = width;
    spec.height = height;
    switch (channels)
    {
        case 3: spec.format = ImageFormat::RGB8; break;
        case 4: spec.format = ImageFormat::RGBA8; break;
    }

    Ref<Texture2D> texture = CreateRef<Texture2D>(data, spec);
    data.release();
    return texture;
}
} // namespace sky