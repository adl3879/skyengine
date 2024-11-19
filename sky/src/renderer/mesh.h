#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

#include "asset_management/asset.h"
#include "material.h"

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
	MaterialID materialID;
};

struct Model : public Asset
{
    Model(std::vector<Mesh> msh) : meshes(msh) {}

    std::vector<Mesh> meshes;
	AssetType getType() const override { return AssetType::Mesh; }
};

using MeshID = UUID;

struct MeshDrawCommand
{
	MeshID meshId;
	glm::mat4 modelMatrix;
};
} // namespace sky