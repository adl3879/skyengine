#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "renderer/texture.h"

namespace sky
{
class TextureSerializer
{
  public:
	bool serialize(const fs::path &path, Ref<Texture2D> texture);
	Ref<Texture2D> deserialize(const fs::path &path);
};
}