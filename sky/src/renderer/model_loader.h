#pragma once

#include "mesh.h"
#include "core/filesystem.h"

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiString;

namespace sky
{
class AssimpModelLoader
{
  public:
	AssimpModelLoader(const fs::path &path);
	~AssimpModelLoader() = default;

	void loadModel(const fs::path &path);
	std::vector<Mesh> getMeshes() const { return meshes; }

  private:
    void ProcessNode(aiNode *node, const aiScene *scene);
    Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene);

  private:
	std::vector<Mesh> meshes;
};
} // namespace sky