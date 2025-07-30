#pragma once

#include "graphics/vulkan/vk_device.h"
#include "graphics/vulkan/vk_types.h"
#include "passes/skybox/brdf_lut.h"
#include "passes/skybox/equirectangular_to_cubemap.h"
#include "passes/skybox/irradiance.h"
#include "passes/skybox/skybox.h"
#include "renderer/passes/skybox/prefilering.h"
#include <skypch.h>

namespace sky 
{
class ImageBasedLighting
{
  public:
    void init(gfx::Device &device);
    void draw(gfx::Device &device, gfx::CommandBuffer cmd, const gfx::AllocatedBuffer &sceneDataBuffer);
    void drawSky(gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        const gfx::AllocatedBuffer &sceneDataBuffer);
    void cleanup(gfx::Device &device);
    void reset();

    void setHdrImageId(ImageID imageId) { m_hdrImageId = imageId; m_dirty = true; }

    auto getIrradianceMapId() { return m_irradiancePass.getCubemapId(); }
    auto getPrefilterMapId() { return m_prefilterEnvmapPass.getCubemapId(); }
    auto getBrdfLutId() { return m_brdfLutPass.getLutId(); }

  private:
    EquirectangularToCubemapPass m_equirectangularToCubemapPass;
    SkyboxPass m_skyboxPass;
    PrefilterEnvmapPass m_prefilterEnvmapPass;
    IrradiancePass m_irradiancePass;
    BrdfLutPass m_brdfLutPass;

    bool m_dirty{false};
    ImageID m_hdrImageId{NULL_IMAGE_ID};
};
}