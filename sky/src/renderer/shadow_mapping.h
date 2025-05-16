#pragma once

#include <skypch.h>

#include <glm/glm.hpp>

namespace sky 
{
glm::mat4 calculateCSMCamera(const std::array<glm::vec3, 8>& frustumCorners,
    const glm::vec3& lightDir,
    float shadowMapSize);
}