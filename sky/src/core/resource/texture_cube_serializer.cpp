#include "texture_cube_serializer.h"

namespace sky 
{
bool TextureCubeSerializer::serialize(const fs::path &path, Ref<TextureCube> texture)
{
    if (texture == nullptr) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        SKY_CORE_ERROR("Failed to open texture file: {0}", path.string());
        return false;
    }

    // Serialize texture metadata
    file.write(reinterpret_cast<const char *>(&texture->width), sizeof(texture->width));
    file.write(reinterpret_cast<const char *>(&texture->height), sizeof(texture->height));
    file.write(reinterpret_cast<const char *>(&texture->channels), sizeof(texture->channels));

    // Write raw float pixel data (4 channels per pixel if STBI_rgb_alpha)
    size_t pixelCount = static_cast<size_t>(texture->width) * texture->height * 4; // RGBA
    size_t totalSize = pixelCount * sizeof(float);
    file.write(reinterpret_cast<const char *>(texture->pixels), totalSize);

    file.close();
    return true;
}

Ref<TextureCube> TextureCubeSerializer::deserialize(const fs::path &path)
{
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open()) {
        SKY_CORE_ERROR("Failed to open binary file: {}", path.string());
        return nullptr;
    }

    auto data = CreateRef<TextureCube>();
    inFile.read(reinterpret_cast<char*>(&data->width), sizeof(data->width));
    inFile.read(reinterpret_cast<char*>(&data->height), sizeof(data->height));
    inFile.read(reinterpret_cast<char*>(&data->channels), sizeof(data->channels));

    size_t pixelCount = static_cast<size_t>(data->width) * data->height * 4;
    data->pixels = new float[pixelCount];
    inFile.read(reinterpret_cast<char*>(data->pixels), pixelCount * sizeof(float));

    inFile.close();
    return data;
}
}