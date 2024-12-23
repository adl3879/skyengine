#include "material_cache.h"

namespace sky
{
const auto getTextureOrElse = [](ImageID imageId, ImageID placeholder) { 
	return imageId != NULL_IMAGE_ID ? imageId : placeholder;
};

void MaterialCache::init(gfx::Device &gfxDevice) 
{
    m_materialDataBuffer.init(gfxDevice,  
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        MAX_MATERIALS * sizeof(MaterialData),
        gfx::FRAME_OVERLAP,
        "materials");
    //m_materialDataCPU.reserve(MAX_MATERIALS);

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

void MaterialCache::upload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd) 
{
    m_materialDataBuffer.uploadNewData(cmd, 
        gfxDevice.getCurrentFrameIndex(), 
        (void *)m_materialDataCPU.data(), 
        sizeof(MaterialData) * m_materialDataCPU.size());
}

void MaterialCache::cleanup(gfx::Device &gfxDevice) 
{
    m_materialDataBuffer.cleanup(gfxDevice);
}

MaterialID MaterialCache::addMaterial(gfx::Device &gfxDevice, Material material) 
{ 
    auto whiteTextureID = gfxDevice.getWhiteTextureID();
    auto id = getFreeMaterialID();

    assert(id < MAX_MATERIALS);

    auto newMaterial = MaterialData{
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
    m_materialDataCPU.push_back(std::move(newMaterial));

    return id;
}

void MaterialCache::updateMaterial(gfx::Device &gfxDevice, MaterialID id, Material material) 
{
    auto whiteTextureID = gfxDevice.getWhiteTextureID();

    auto &mat = m_materialDataCPU.at(id);
    mat = MaterialData{
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
}

const MaterialData &MaterialCache::getMaterial(MaterialID id) const
{
    return m_materialDataCPU.at(id);
}

MaterialID MaterialCache::getFreeMaterialID() const 
{
    return m_materialDataCPU.size();
}
} // namespace sky