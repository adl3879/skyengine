#include "material_cache.h"

namespace sky
{
void MaterialCache::init(gfx::Device &gfxDevice) 
{
    m_materialDataBuffer =
        gfxDevice.createBuffer(MAX_MATERIALS * sizeof(MaterialData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_AUTO);
    //vkutil::addDebugLabel(gfxDevice.getDevice(), materialDataBuffer.buffer, "material data");

    {   // create default normal map texture
        std::uint32_t normal = 0xFFFF8080; // (0.5, 0.5, 1.0, 1.0)
        m_defaultNormalMapTextureID = gfxDevice.createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            }, &normal);
    }
}

void MaterialCache::cleanup(gfx::Device &gfxDevice) 
{
    gfxDevice.destroyBuffer(m_materialDataBuffer);
}

MaterialID MaterialCache::addMaterial(gfx::Device &gfxDevice, Material material) 
{
    const auto getTextureOrElse = [](ImageID imageId, ImageID placeholder) { 
        return imageId != NULL_IMAGE_ID ? imageId : placeholder;
    };

    // store on GPU
    MaterialData *data = (MaterialData *)m_materialDataBuffer.info.pMappedData;
    auto whiteTextureID = gfxDevice.getWhiteTextureID();
    auto id = getFreeMaterialID();
    assert(id < MAX_MATERIALS);
    data[id] = MaterialData{
        .baseColor = material.baseColor,
        .metalRoughnessEmissive =
            glm::vec4{material.metallicFactor, material.roughnessFactor, material.emissiveFactor, 0.f},
        .albedoTex = getTextureOrElse(material.albedoTexture, whiteTextureID),
        .normalTex = getTextureOrElse(material.normalMapTexture, m_defaultNormalMapTextureID),
        .metallicTex = getTextureOrElse(material.metallicTexture, whiteTextureID),
        .roughnessTex = getTextureOrElse(material.roughnessTexture, whiteTextureID),
        .ambientOcclusionTex = getTextureOrElse(material.ambientOcclusionTexture, whiteTextureID),
        .emissiveTex = getTextureOrElse(material.emissiveTexture, whiteTextureID),
    };

    // store on CPU
    m_materials[id] = std::move(material);

    return id;
}

const Material &MaterialCache::getMaterial(MaterialID id) const
{
    return m_materials.at(id);
}

MaterialID MaterialCache::getFreeMaterialID() const 
{
    return UUID::generate();
}
} // namespace sky