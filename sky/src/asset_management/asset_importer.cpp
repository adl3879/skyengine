#include "asset_importer.h"

#include "mesh_importer.h"
#include "texture_importer.h"
#include "scene_importer.h"

namespace sky
{
using AssetImportFn = std::function<Ref<Asset>(AssetHandle, AssetMetadata &)>;

static std::unordered_map<AssetType, AssetImportFn> s_assetImportFns = {
    {AssetType::Mesh,       MeshImporter::importAsset},
    {AssetType::Texture2D,  TextureImporter::importAsset},
    {AssetType::Scene,      SceneImporter::importAsset}
};

Ref<Asset> AssetImporter::importAsset(AssetHandle handle, AssetMetadata &metadata) 
{
    if (s_assetImportFns.find(metadata.type) == s_assetImportFns.end())
    {
        SKY_CORE_ERROR("No importer available for asset type: {}", static_cast<uint16_t>(metadata.type));
        return nullptr;
    }

    return s_assetImportFns.at(metadata.type)(handle, metadata);
}
} // namespace sky