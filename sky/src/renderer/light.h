#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

#include "core/color.h"
#include "core/helpers/yaml.h"

namespace sky
{
enum class LightType
{
    None,
    Directional,
    Point,
    Spot,
};

using LightID = uint32_t;
struct Light
{
    LightID id;
    LightType type{LightType::None};

    LinearColor color;
    float range{0.f};
    float intensity{0.f};
    bool castShadow{true};

    glm::vec2 scaleOffset; // precomputed for spot lights (for angle attenuation)
    float innerConeAngle, outerConeAngle;

    void setConeAngles(float innerConeAngle, float outerConeAngle); 
    int getShaderType() const;

    void serialize(YAML::Emitter &out); 
    void deserialize(YAML::detail::iterator_value entity);
};

// Representation of light data on GPU (see lighting.glsl)
struct GPULightData
{
    glm::vec3 position;
    std::uint32_t type;
    glm::vec3 direction;
    float range;
    LinearColorNoAlpha color;
    float intensity;
    glm::vec2 scaleOffset;
    float shadowMapIndex;
    float unused;
};
}