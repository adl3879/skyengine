#pragma once

#include <skypch.h>
#include <vulkan/vulkan_core.h>

#include "vk_types.h"
#include "vk_bindless_set.h"

namespace sky::gfx
{
class Device;

class ImageCache
{
  public:
	ImageCache(Device& device);

	ImageID addImage(AllocatedImage image);
	ImageID addImage(ImageID id, AllocatedImage image);
	const AllocatedImage &getImage(ImageID id) const;

	ImageID getFreeImageID() const;

	void destroyImages();

	BindlessSetManager bindlessSetManager;

  private:
	std::vector<AllocatedImage> m_images;
    gfx::Device &m_device;
};
} // namespace sky::gfx