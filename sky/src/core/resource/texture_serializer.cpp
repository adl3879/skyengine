#include "texture_serializer.h"

namespace sky
{
bool TextureSerializer::serialize(const fs::path &path, Ref<Texture2D> texture) 
{
    if (texture == nullptr) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        SKY_CORE_ERROR("Failed to open mesh file: {0}", path.string());
        return false;
    }

    // Serialize texture metadata
    uint32_t width = texture->width;
    uint32_t height = texture->height;
    uint32_t channels = texture->channels;
    file.write(reinterpret_cast<const char *>(&width), sizeof(width));
    file.write(reinterpret_cast<const char *>(&height), sizeof(height));
    file.write(reinterpret_cast<const char *>(&channels), sizeof(channels));

    // Serialize texture data
    unsigned char *data = texture->pixels;
    size_t dataSize = width * height * channels;
    file.write(reinterpret_cast<const char *>(data), dataSize);

    file.close();
    return true;
}

Ref<Texture2D> TextureSerializer::deserialize(const fs::path &path) 
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        SKY_CORE_ERROR("Failed to open texture file: {0}", path.string());
        return nullptr;
    }

    // Read metadata
    uint32_t width, height, channels;
    file.read(reinterpret_cast<char *>(&width), sizeof(width));
    file.read(reinterpret_cast<char *>(&height), sizeof(height));
    file.read(reinterpret_cast<char *>(&channels), sizeof(channels));

    // Read texture data
    size_t dataSize = width * height * channels;
    std::vector<unsigned char> data(dataSize);
    file.read(reinterpret_cast<char *>(data.data()), dataSize);

    auto texture = CreateRef<Texture2D>();
    texture->width = width;
    texture->height = height;
    texture->channels = channels;

    // Allocate new memory and copy the data
    texture->pixels = new unsigned char[dataSize];
    std::memcpy(texture->pixels, data.data(), dataSize);

    file.close();
    return texture;
}
} // namespace sky