#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "renderer/material.h"

namespace sky
{
class MaterialSerializer
{
  public:
	bool serialize(const fs::path &path, const Material &mat);
	Material deserialize(const fs::path &path);
};
}