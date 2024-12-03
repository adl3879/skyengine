#include "texture_importer.h"

#include "core/project_management/project_manager.h"
#include "core/resource/import_data.h"
#include "core/resource/texture_serializer.h"

#include <stb_image.h>

namespace sky
{
static bool loadTextureFromSrc(const fs::path &src, const fs::path &dst) 
{
	// load texture from file
	auto texture = TextureImporter::loadTexture(src);
   
	TextureSerializer serializer;
	if (serializer.serialize(dst, texture))
    {  
		//SKY_CORE_INFO("Successfully wrote texture: {} to disk", src.string());
		return true;
	}
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
	std::string dstFileName = std::format("{0}-{1}.texture", path.filename().string(), uuid.toString());
	data.destination = ProjectManager::getConfig().getImportedCachePath() / dstFileName;

	ImportDataSerializer dataSerializer(data);
	dataSerializer.serialize(path.string() + ".import");

	return loadTextureFromSrc(data.source, data.destination);
}

Ref<Texture2D> TextureImporter::importAsset(AssetHandle handle, AssetMetadata &metadata)
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

	TextureSerializer serializer;
	auto asset = serializer.deserialize(data.destination);
	if (asset != nullptr) SKY_CORE_INFO("Texture: {} successfully loaded", path.string());
	
	return asset;
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
        return nullptr;
    }

    data->channels = 4;
    return data;
}

Ref<Texture2D> TextureImporter::loadTexture(const void *buffer, uint64_t length) 
{
	auto data = CreateRef<Texture2D>();

    data->pixels = stbi_load_from_memory((const stbi_uc *)buffer, length, &data->width, &data->height, &data->channels, 4);
    
	data->channels = 4;
    return data;
}
} // namespace sky