#include "model_loader.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "core/project_management/project_manager.h"

namespace sky
{
// Helper function to get a texture path
fs::path AssimpModelLoader::getTexturePath(aiMaterial *material, aiTextureType type)
{
    aiString path;
    if (material->GetTexture(type, 0, &path) == AI_SUCCESS)
    {
        auto p = fs::relative(m_path.parent_path(), ProjectManager::getConfig().getAssetDirectory());
        return p / path.C_Str();
    }
    return ""; // Return an empty string if the texture is not found
}

AssimpModelLoader::AssimpModelLoader(const fs::path &path) 
{
    loadModel(path);
}

void AssimpModelLoader::loadModel(const fs::path &path)
{
    m_path = path;
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
    processNode(scene->mRootNode, scene);
    importer.FreeScene();
}

void AssimpModelLoader::processNode(aiNode *node, const aiScene *scene) 
{
    // Process all the node's meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		m_meshes.push_back(processMesh(mesh, scene));
	}

	// Process all the node's children
	for (unsigned int i = 0; i < node->mNumChildren; i++) 
	{
		processNode(node->mChildren[i], scene);
	}
}

MeshLoaderReturn AssimpModelLoader::processMesh(aiMesh *mesh, const aiScene *scene) 
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
    processedMesh.name = mesh->mName.C_Str();

    MaterialPaths materialPaths;
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        materialPaths = extractMaterialPaths(material);
    }

    return {.materialPaths = materialPaths, .mesh = processedMesh};
}

MaterialPaths AssimpModelLoader::extractMaterialPaths(aiMaterial* material)
{
    MaterialPaths materialPaths;
    materialPaths.albedoTexture = getTexturePath(material, aiTextureType_DIFFUSE);
    materialPaths.normalMapTexture = getTexturePath(material, aiTextureType_NORMALS);
    materialPaths.metallicsTexture = getTexturePath(material, aiTextureType_METALNESS); // Requires Assimp 5.0+
    materialPaths.roughnessTexture = getTexturePath(material, aiTextureType_DIFFUSE_ROUGHNESS);
    materialPaths.roughnessTexture = getTexturePath(material, aiTextureType_DIFFUSE_ROUGHNESS);
    materialPaths.ambientOcclusionTexture = getTexturePath(material, aiTextureType_LIGHTMAP);
    materialPaths.emissiveTexture = getTexturePath(material, aiTextureType_EMISSIVE);

    return materialPaths;
}
} // namespace sky