#pragma once

#include <skypch.h>

#include <glm/glm.hpp>
#include "graphics/vulkan/vk_types.h"
#include "core/color.h"
#include "core/uuid.h"

namespace sky
{
struct MaterialData
{
    LinearColor baseColor;
    glm::vec4   metalRoughnessEmissive;
    uint32_t    albedoTex;
    uint32_t    normalTex;
    uint32_t    metallicTex;
    uint32_t    roughnessTex;
    uint32_t    ambientOcclusionTex;
    uint32_t    emissiveTex;
};

struct Material
{
    LinearColor baseColor{1.f, 1.f, 1.f, 1.f};
    float       metallicFactor{0.f};
    float       roughnessFactor{0.7f};
    float       emissiveFactor{0.f};

    ImageID     albedoTexture{NULL_IMAGE_ID};
    ImageID     normalMapTexture{NULL_IMAGE_ID};
    ImageID     metallicTexture{NULL_IMAGE_ID};
    ImageID     roughnessTexture{NULL_IMAGE_ID};
    ImageID     ambientOcclusionTexture{NULL_IMAGE_ID};
    ImageID     emissiveTexture{NULL_IMAGE_ID};

    std::string name;
};

using MaterialID = UUID;
}