#pragma once

#include <skypch.h>
#include "asset.h"
#include "renderer/material.h"

namespace sky
{
class MaterialImporter
{
  public:
	static Ref<MaterialAsset> importAsset(AssetHandle handle, AssetMetadata &metadata);
};
}