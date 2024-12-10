#include "scene_importer.h"

#include "scene/scene_serializer.h"
#include "core/project_management/project_manager.h"

namespace sky
{
Ref<Scene> SceneImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
{
	auto path = ProjectManager::getConfig().getAssetDirectory() / metadata.filepath;
	auto scene = CreateRef<Scene>();
	SceneSerializer serializer(scene);
	if (serializer.deserialize(path)) SKY_CORE_INFO("Scene: {} successfully loaded", path.string());
	return scene;
}
}
