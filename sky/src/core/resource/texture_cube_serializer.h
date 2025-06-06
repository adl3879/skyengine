#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "renderer/texture.h"

namespace sky
{
class TextureCubeSerializer
{
  public:
	bool serialize(const fs::path &path, Ref<TextureCube> texture);
	Ref<TextureCube> deserialize(const fs::path &path);
};
}