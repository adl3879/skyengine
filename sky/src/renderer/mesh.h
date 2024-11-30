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
	std::vector<Vertex>		vertices;
	std::vector<uint32_t>	indices;
	MaterialID				material;
	std::string				name;
};

using MeshID = UUID;
struct Model : public Asset
{
    Model(std::vector<MeshID> msh) : meshes(msh) {}

    std::vector<MeshID> meshes;
	AssetType getType() const override { return AssetType::Mesh; }
};

struct MeshDrawCommand
{
	MeshID		meshId;
	glm::mat4	modelMatrix;
	bool		isVisible;
};
} // namespace sky