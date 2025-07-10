#pragma once

#include <skypch.h>

#include "renderer/passes/pass.h"

namespace sky 
{
class CSMPass : public Pass
{
  public:
    static constexpr int NUM_SHADOW_CASCADES = 3;

  public:
    void init(const gfx::Device &device, const std::array<float, NUM_SHADOW_CASCADES>& percents);
    void draw(const gfx::Device &device,
        gfx::CommandBuffer cmd,
        Camera &camera,
        const gfx::AllocatedBuffer &sceneDataBuffer,
        bool shadowsEnabled);
    void cleanup(const gfx::Device &device);

    ImageID getShadowMap() { return m_csmShadowMapId; }

    std::array<float, NUM_SHADOW_CASCADES> cascadeFarPlaneZs{};
    std::array<glm::mat4, NUM_SHADOW_CASCADES> csmLightSpaceTMs{};

    // how cascades are distributed
    std::array<float, NUM_SHADOW_CASCADES> percents;

  private:
    ImageID m_csmShadowMapId;

  private:
    struct PushConstants
    {

    };
};
}