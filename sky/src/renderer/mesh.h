#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

namespace sky
{
struct Vertex
{
	glm::vec3 position;
    float uv_x;
	glm::vec3 normal;
    float uv_y;
	glm::vec4 tangent;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};
using MeshId = uint32_t;

struct MeshDrawCommand
{
	MeshId meshId;
	glm::mat4 modelMatrix;
};
} // namespace sky