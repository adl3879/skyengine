#include "model_loader.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

namespace sky
{
AssimpModelLoader::AssimpModelLoader(const fs::path &path) 
{
    loadModel(path);
}

void AssimpModelLoader::loadModel(const fs::path &path)
{
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        SKY_CORE_ERROR("Error loading model: {0}", importer.GetErrorString());
        return;
    }

    // Process the root node recursively
    ProcessNode(scene->mRootNode, scene);
    importer.FreeScene();
}

void AssimpModelLoader::ProcessNode(aiNode *node, const aiScene *scene) 
{
    // Process all the node's meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(mesh, scene));
	}

	// Process all the node's children
	for (unsigned int i = 0; i < node->mNumChildren; i++) 
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

Mesh AssimpModelLoader::ProcessMesh(aiMesh *mesh, const aiScene *scene) 
{
    Mesh processedMesh;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        // Position
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

        // Normals
        if (mesh->HasNormals())
        {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        }
        if (mesh->mTangents)
        {
            vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f};
        }
        if (mesh->mBitangents)
        {
            //vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
        }
        // Texture coordinates
        if (mesh->mTextureCoords[0])
        {
            vertex.uv_x = mesh->mTextureCoords[0][i].x;
            vertex.uv_y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.uv_x = 0.0f;
            vertex.uv_y = 0.0f;
        }

        vertices.push_back(vertex);
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Store the data in the mesh
    processedMesh.vertices = std::move(vertices);
    processedMesh.indices = std::move(indices);

    return processedMesh;
}
} // namespace sky