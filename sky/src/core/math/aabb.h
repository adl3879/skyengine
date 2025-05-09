#pragma once

#include <skypch.h>
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace sky::math
{
struct AABB
{
    glm::vec3 min;
    glm::vec3 max;

    glm::vec3 calculateSize() const { return glm::abs(max - min); }
};
} // namespace math
