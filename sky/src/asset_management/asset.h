#pragma once

#include <skypch.h>
#include "core/uuid.h"
#include "core/filesystem.h"

namespace sky
{
using AssetHandle = UUID;

enum class AssetType : uint16_t
{
    None = 0,
    Mesh,
    Texture2D,
    Scene,
    Material,
    Shader,
    Folder
};

struct AssetMetadata
{
    AssetType type = AssetType::None;
    AssetHandle handle = NULL_UUID;
    fs::path filepath;
    bool isLoaded = false;
    std::vector<AssetHandle> dependencies;

    operator bool() const { return type != AssetType::None; }
};

std::string_view assetTypeToString(AssetType type);
AssetType assetTypeFromString(std::string_view assetType);

class Asset
{
  public:
    AssetHandle handle;

    [[nodiscard]] virtual AssetType getType() const = 0;
};
}