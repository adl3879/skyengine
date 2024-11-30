#pragma once

#include "mesh.h"
#include "core/filesystem.h"

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiString;
struct aiMaterial;
enum aiTextureType;

namespace sky
{
struct MeshLoaderReturn
{
    MaterialPaths	materialPaths;
    Mesh			mesh;
	std::string		materialName;
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
	fs::path getTexturePath(aiMaterial *material, aiTextureType type);

  private:
	std::vector<MeshLoaderReturn> m_meshes;
	fs::path m_path;
};
} // namespace sky