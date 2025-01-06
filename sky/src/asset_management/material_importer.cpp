#include "material_importer.h"

#include "core/resource/material_serializer.h"
#include "core/project_management/project_manager.h"
#include "core/application.h"

namespace sky
{
Ref<MaterialAsset> MaterialImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
{
	auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;
	MaterialSerializer serializer;
	auto material = serializer.deserialize(path);
	material.name = metadata.filepath.string();
	auto id = Application::getRenderer()->addMaterialToCache(material);

	return CreateRef<MaterialAsset>(id);
}
} // namespace sky