#include "mesh_importer.h"

#include "renderer/model_loader.h"
#include "core/project_management/project_manager.h"
#include "asset_manager.h"
#include "renderer/texture.h"
#include "core/application.h"
#include "core/resource/import_data.h"
#include "core/resource/mesh_serializer.h"

namespace sky
{
static bool loadAssimpModel(const fs::path &src, const fs::path &dst) 
{
	AssimpModelLoader modelLoader(src);
	MeshSerializer meshSerializer;
	if (meshSerializer.serialize(dst, modelLoader.getMeshes()))
    {  
		SKY_CORE_INFO("Successfully wrote model: {} to disk", src.string());
		return true;
	}
    else
    {
        SKY_CORE_ERROR("Failed to write model: {} to disk", src.string());
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
	std::string dstFileName = std::format("{0}-{1}.mesh", path.filename().string(), uuid.toString());
	data.destination = ProjectManager::getConfig().getImportedCachePath() / dstFileName;

	ImportDataSerializer dataSerializer(data);
	dataSerializer.serialize(path.string() + ".import");

	return loadAssimpModel(data.source, data.destination);
}

Ref<Model> MeshImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
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
		loadAssimpModel(data.source, data.destination);	
    }

	return loadModel(handle, data.destination);
}

Ref<Model> MeshImporter::loadModel(AssetHandle handle, const fs::path &path) 
{
	MeshSerializer serializer;
    std::vector<MeshID> meshes;
	auto renderer = Application::getRenderer();

	for (auto &mesh : serializer.deserialize(path, handle))
	{
		// add mesh to cache
		auto meshID = renderer->addMeshToCache(mesh);
		meshes.push_back(meshID);
	}
	SKY_CORE_INFO("Model: {} loaded successfully", path.string());

	return CreateRef<Model>(meshes);
}
} // namespace sky