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

enum ModelType
{
	Custom,
	Cube,
	Sphere,
	Plane,
	Cylinder,
	Taurus,
	Cone,
	Capsule,
};

static std::string modelTypeToString(ModelType type)
{
    switch (type)
    {
        case ModelType::Custom: return "Custom";
        case ModelType::Cube: return "Cube";
        case ModelType::Sphere: return "Sphere";
        case ModelType::Plane: return "Plane";
        case ModelType::Cylinder: return "Cylinder";
        case ModelType::Taurus: return "Taurus";
        case ModelType::Cone: return "Cone";
        case ModelType::Capsule: return "Capsule";
        default: return "Unknown";
    }
}

static ModelType stringToModelType(const std::string &str)
{
    if (str == "Custom") return ModelType::Custom;
    if (str == "Cube") return ModelType::Cube;
    if (str == "Sphere") return ModelType::Sphere;
    if (str == "Plane") return ModelType::Plane;
    if (str == "Cylinder") return ModelType::Cylinder;
    if (str == "Taurus") return ModelType::Taurus;
    if (str == "Cone") return ModelType::Cone;
    if (str == "Capsule") return ModelType::Capsule;
    return ModelType::Custom; // Default fallback
}

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
	MeshID		 meshId;
	glm::mat4	 modelMatrix;
	bool		 isVisible;
	uint32_t	 uniqueId = 0;
    math::Sphere worldBoundingSphere;
    MaterialID   material = NULL_MATERIAL_ID;
};
} // namespace sky