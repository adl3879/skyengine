#include "asset_importer.h"

#include "asset_management/asset.h"
#include "mesh_importer.h"
#include "texture_importer.h"
#include "scene_importer.h"
#include "material_importer.h"
#include "texture_cube_importer.h"

namespace sky
{
using AssetImportFn = std::function<Ref<Asset>(AssetHandle, AssetMetadata &)>;

static std::unordered_map<AssetType, AssetImportFn> s_assetImportFns = {
    {AssetType::Mesh,           MeshImporter::importAsset},
    {AssetType::Texture2D,      TextureImporter::importAsset},
    {AssetType::TextureCube,    TextureCubeImporter::importAsset},
    {AssetType::Scene,          SceneImporter::importAsset},
    {AssetType::Material,       MaterialImporter::importAsset},
};

Ref<Asset> AssetImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
{
    if (s_assetImportFns.find(metadata.type) == s_assetImportFns.end())
    {
        SKY_CORE_ERROR("No importer available for asset type: {}, file: {}", 
            assetTypeToString(metadata.type), metadata.filepath.string());
        return nullptr;
    }

    return s_assetImportFns.at(metadata.type)(handle, metadata);
}
} // namespace sky