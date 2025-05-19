#include "image.h"

#include "core/application.h"
#include "asset_management/texture_importer.h"

namespace sky
{
namespace helper
{
ImageID loadImageFromFile(const fs::path& path, float scaleFactor)
{
    auto renderer = Application::getRenderer();
    auto tex = TextureImporter::loadTexture(path);
    
    uint32_t width = static_cast<uint32_t>(tex->width * scaleFactor);
    uint32_t height = static_cast<uint32_t>(tex->height * scaleFactor);

    auto texture = renderer->createImage(
        {
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .extent =
                VkExtent3D{
                    .width = width,
                    .height = height,
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

ImageID loadImageFromTexture(Ref<Texture2D> tex, VkFormat format, VkImageUsageFlags usage, bool mipMap)
{
    if (tex == nullptr) return NULL_IMAGE_ID;

    if (tex->vkImageID == NULL_IMAGE_ID)
    {
		auto renderer = Application::getRenderer();
		tex->vkImageID = renderer->createImage(
			{
				.format = format,
				.usage = usage |                           //
						 VK_IMAGE_USAGE_TRANSFER_DST_BIT | // for uploading pixel data to image
						 VK_IMAGE_USAGE_TRANSFER_SRC_BIT,  // for generating mips
				.extent =
					VkExtent3D{
						.width = (std::uint32_t)tex->width,
						.height = (std::uint32_t)tex->height,
						.depth = 1,
					},
				.mipMap = mipMap,
			},
			tex->pixels);
    }
	return tex->vkImageID;
}
}
}