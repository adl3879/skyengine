#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "graphics/vulkan/vk_types.h"

namespace sky
{
namespace helper
{
ImageID loadImageFromFile(const fs::path& path);
ImageID loadImageFromData(const void *buffer, uint64_t length);
}
}