#include "image.h"

#include "core/application.h"
#include "asset_management/texture_importer.h"

namespace sky
{
namespace helper
{
ImageID loadImageFromFile(const fs::path& path)
{
    auto renderer = Application::getRenderer();
    auto tex = TextureImporter::loadTexture(path);
    auto texture = renderer->createImage(
        {
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent =
                VkExtent3D{
                    .width = static_cast<uint32_t>(tex->width),
                    .height = static_cast<uint32_t>(tex->height),
                    .depth = 1,
                },
            .mipMap = true,
        },
        tex->pixels);
    return texture;
}

ImageID loadImageFromData(const void *buffer, uint64_t length) 
{
	auto renderer = Application::getRenderer();
    auto tex = TextureImporter::loadTexture(buffer, length);
    auto texture = renderer->createImage(
        {
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent =
                VkExtent3D{
                    .width = static_cast<uint32_t>(tex->width),
                    .height = static_cast<uint32_t>(tex->height),
                    .depth = 1,
                },
            .mipMap = true,
        },
        tex->pixels);
    return texture;
}
}
}