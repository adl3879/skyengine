#pragma once

#include <skypch.h>

#include "light.h"
#include "graphics/vulkan/vk_NBuffer.h"
#include "graphics/vulkan/vk_device.h"
#include "core/uuid.h"
#include "core/transform/transform.h"

namespace sky
{
class LightCache
{
  public:
    void init(gfx::Device &gfxDevice);
    void upload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd);
    LightID addLight(const Light &light, const Transform &transform);
    void updateLight(LightID id, const Light &light, const Transform &transform);

    GPULightData getLight(LightID id);
    LightID getFreeLightID();

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