#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "graphics/vulkan/vk_device.h"
#include "material.h"

namespace sky
{
class MaterialCache
{
  public:
    void init(gfx::Device &gfxDevice);
    void cleanup(gfx::Device &gfxDevice);

    MaterialID addMaterial(gfx::Device &gfxDevice, Material material);
    const Material &getMaterial(MaterialID id) const;

    MaterialID getFreeMaterialID() const;

    const gfx::AllocatedBuffer &getMaterialDataBuffer() const { return m_materialDataBuffer; }
    VkDeviceAddress getMaterialDataBufferAddress() const { return m_materialDataBuffer.address; }

  private:
    std::unordered_map<MaterialID, Material> m_materials;

    static const auto MAX_MATERIALS = 1000;
    gfx::AllocatedBuffer m_materialDataBuffer;

    ImageID m_defaultNormalMapTextureID{NULL_IMAGE_ID};
};
}