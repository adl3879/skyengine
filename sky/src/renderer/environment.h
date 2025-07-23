#pragma once

#include "asset_management/asset.h"
#include <skypch.h>

#include <glm/glm.hpp>

namespace sky
{
struct SkyAtmosphere
{
	float       time{24.f};
	glm::vec3   groundAlbedo{glm::vec3{0.1f}};
	float       mieScatteringBase{1.996f};
	glm::vec3   rayleighScatteringBase{5.802f, 13.558f, 43.1f};
	float       mieAbsorptionBase{4.4f};
	glm::vec3   ozoneAbsorptionBase{0.650f, 1.881f, 0.085f};
	float       exposure{20.f};
	float       rayleighAbsorptionBase{0.f}; 
};

struct Environment
{
    AssetHandle skyboxHandle{NULL_UUID};
};
}