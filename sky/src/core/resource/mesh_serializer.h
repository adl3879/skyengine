#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "renderer/model_loader.h"

namespace sky
{
class MeshSerializer
{
  public:
	bool serialize(const fs::path &path, std::vector<MeshLoaderReturn> meshes);
	std::vector<Mesh> deserialize(const fs::path &path, AssetHandle handle = NULL_UUID);
};
}