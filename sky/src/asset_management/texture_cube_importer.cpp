#include "texture_cube_importer.h"
#include "core/resource/import_data.h"
#include "core/resource/texture_cube_serializer.h"
#include "renderer/texture.h"
#include "core/project_management/project_manager.h"
#include "skypch.h"

#include <stb_image.h>

namespace sky 
{
static bool loadTextureFromSrc(const fs::path &src, const fs::path &dst)
{
    // load texture from file
	auto texture = TextureCubeImporter::loadTexture(src);
   
	TextureCubeSerializer serializer;
	if (serializer.serialize(dst, texture)) return true;
    else
    {
        SKY_CORE_ERROR("Failed to write texture: {} to disk", src.string());
		return false;
	}
}

static bool createImportFile(const fs::path &path)
{
    ImportData data;
	data.id = UUID::generate();
	data.type = AssetType::Mesh;
	data.source = path;

	auto uuid = UUID::generate();
	std::string dstFileName = std::format("{0}-{1}.texture3d", path.filename().string(), uuid.toString());
	data.destination = ProjectManager::getConfig().getImportedCachePath() / dstFileName;

	ImportDataSerializer dataSerializer(data);
	dataSerializer.serialize(path.string() + ".import");

	return loadTextureFromSrc(data.source, data.destination);
}

Ref<TextureCube> TextureCubeImporter::importAsset(AssetHandle handle, AssetMetadata &metadata)
{
    const auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;

    auto importDataFile = metadata.filepath.string() + ".import";
    if (!fs::exists(ProjectManager::getConfig().getAssetDirectory() / importDataFile))
    {
        createImportFile(path);
    }

	ImportData data;
	ImportDataSerializer dataSerializer(data);
	dataSerializer.deserialize(ProjectManager::getConfig().getAssetDirectory() / importDataFile);

	if (!fs::exists(data.destination))
    {
		loadTextureFromSrc(data.source, data.destination);    
	}

	TextureCubeSerializer serializer;
	auto asset = serializer.deserialize(data.destination);
	if (asset != nullptr) SKY_CORE_INFO("Texture: {} successfully loaded", path.string());
	
	return asset;
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