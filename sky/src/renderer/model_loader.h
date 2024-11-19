#pragma once

#include "mesh.h"
#include "core/filesystem.h"

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiString;
struct aiMaterial;

namespace sky
{
struct MaterialPaths
{
	std::string     albedoTexture;
	std::string     normalMapTexture;
	std::string     metallicsTexture;
	std::string     roughnessTexture;
	std::string     ambientOcclusionTexture;
	std::string     emissiveTexture;
};

struct MeshLoaderReturn
{
    MaterialPaths	materialPaths;
    Mesh			mesh;
};

class AssimpModelLoader
{
  public:
	AssimpModelLoader(const fs::path &path);
	~AssimpModelLoader() = default;

	void loadModel(const fs::path &path);
	std::vector<MeshLoaderReturn> getMeshes() const { return m_meshes; }

  private:
    void processNode(aiNode *node, const aiScene *scene);
    MeshLoaderReturn processMesh(aiMesh *mesh, const aiScene *scene);
	MaterialPaths extractMaterialPaths(aiMaterial *material);

  private:
	std::vector<MeshLoaderReturn> m_meshes;
};
} // namespace sky