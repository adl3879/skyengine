#pragma once

#include <skypch.h>

#include "light.h"
#include "graphics/vulkan/vk_NBuffer.h"
#include "graphics/vulkan/vk_device.h"
#include "core/transform/transform.h"

namespace sky
{
class LightCache
{
  public:
    void init(gfx::Device &gfxDevice);
    void updateAndUpload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd, 
        const std::vector<std::pair<Light, Transform>>& lights); 

    auto getSunlightIndex() const { return m_sunlightIndex; }
    auto getBuffer() const { return m_lightDataBuffer.getBuffer(); }
    auto getSize() const { return m_lightDataCPU.size(); }

  private:
    gfx::NBuffer m_lightDataBuffer;

	static const int MAX_LIGHTS = 100;
    std::vector<GPULightData> m_lightDataCPU;
    const float m_pointLightMaxRange{25.f};
    const float m_spotLightMaxRange{64.f};
    std::int32_t m_sunlightIndex{-1}; // index of sun light inside the light data buffer
};
}