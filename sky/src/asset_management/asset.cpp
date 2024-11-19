#include "assert.h"
#include "asset.h"

namespace sky
{
std::string_view sky::assetTypeToString(AssetType type) 
{
    switch (type)
    {
        case AssetType::None: return "AssetType::None";
        case AssetType::Mesh: return "AssetType::Mesh";
        case AssetType::Texture2D: return "AssetType::Texture2D";
        default: break;
    }
    return "AssetType::<Invalid>";
}

AssetType assetTypeFromString(std::string_view assetType) 
{
    if (assetType == "AssetType::Mesh") return AssetType::Mesh;
    if (assetType == "AssetType::Texture2D") return AssetType::Texture2D;

    return AssetType::None;
}
} // namespace sky