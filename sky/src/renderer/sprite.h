#pragma once

#include <skypch.h>

#include <glm/glm.hpp>
#include "graphics/vulkan/vk_types.h"

namespace sky
{
struct QuadVertex
{
	glm::vec2	position;
	glm::vec2	texCoord;
	glm::vec4	color;
	ImageID		textureId;
	uint32_t    uniqueId;
};

struct Sprite
{
    glm::vec2	position{-0.5f, -0.5f};
    glm::vec2	size{1.f, 1.f};
    glm::vec4	color{1.f, 1.f, 1.f, 1.f};
	float		rotation{0.f};
	glm::vec2	origin{-0.5f, -0.5f};
	ImageID		textureId;
	glm::vec2	texCoord{0.f, 0.f};
	uint32_t	uniqueId;
};
}