#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "graphics/vulkan/vk_device.h"
#include "graphics/vulkan/vk_NBuffer.h"
#include "material.h"

namespace sky
{
class MaterialCache
{
  public:
    void init(gfx::Device &gfxDevice);
    void upload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd);
    void cleanup(gfx::Device &gfxDevice);

    MaterialID addMaterial(gfx::Device &gfxDevice, Material material);
    const Material &getMaterial(MaterialID id) const;
    MaterialID getDefaultMaterial() const { return m_defaultMaterial; }
    void updateMaterial(gfx::Device &gfxDevice, MaterialID id, Material material);

    MaterialID getFreeMaterialID() const;

    VkDeviceAddress getMaterialDataBufferAddress() const { return m_materialDataBuffer.getBuffer().address; }

  private:
    std::vector<Material> m_materialDataCPU;
    std::vector<MaterialData> m_materialDataGPU;

    static const auto MAX_MATERIALS = 1000;
    gfx::NBuffer m_materialDataBuffer;

    ImageID m_defaultNormalMapTextureID{NULL_IMAGE_ID};
    MaterialID m_defaultMaterial;
};
}