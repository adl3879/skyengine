#include "vk_image_cache.h"

#include "vk_device.h"

namespace sky::gfx
{
ImageCache::ImageCache(Device &device): m_device(device) 
{
}

ImageID ImageCache::addImage(AllocatedImage image) 
{
    return addImage(getFreeImageID(), image);
}

ImageID ImageCache::addImage(ImageID id, AllocatedImage image) 
{
    if (id != m_images.size())
        m_images[id] = image; // replacing existing image
    else
        m_images.push_back(image);

    bindlessSetManager.addImage(m_device.getDevice(), id, image.imageView);

    return id;
}

const AllocatedImage &ImageCache::getImage(ImageID id) const
{
    return m_images.at(id);
}

ImageID ImageCache::getFreeImageID() const 
{
	return m_images.size();
}

void ImageCache::destroyImages() 
{
    for (const auto &image : m_images)
    {
        m_device.destroyImage(image);
    }
    m_images.clear();
}
} // namespace sky::gfx