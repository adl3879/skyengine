#include "model_loader.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "core/project_management/project_manager.h"

namespace sky
{
// Helper function to get a texture path
fs::path AssimpModelLoader::getTexturePath(const aiScene *scene, 
    aiMaterial *material, 
    aiTextureType type, 
    const std::string &matParam)
{
    aiString path;
    if (material->GetTexture(type, 0, &path) == AI_SUCCESS)
    {
        // Check if the texture is embedded
        if (path.C_Str()[0] == '*')
        {
            // Embedded texture, extract the index
            int index = std::atoi(path.C_Str() + 1);
            if (index >= 0 && index < scene->mNumTextures)
            {
                const aiTexture *texture = scene->mTextures[index];
                return saveEmbeddedTexture(texture, matParam);
            }
        }
        else
        {
            // External texture
            auto p = fs::relative(m_path.parent_path(), ProjectManager::getConfig().getAssetDirectory());
            return p / path.C_Str();
        }
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
    auto importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FixInfacingNormals |
                       aiProcess_CalcTangentSpace | aiProcess_OptimizeGraph;
    const aiScene *scene = importer.ReadFile(path.string(), importFlags);

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
            vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 0.0f};
        }
        if (mesh->mBitangents)
        {
            //vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z, 0.0f};
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
        materialPaths = extractMaterialPaths(scene, material);
    }

    return {.materialPaths = materialPaths, .mesh = processedMesh};
}

MaterialPaths AssimpModelLoader::extractMaterialPaths(const aiScene *scene, aiMaterial* material)
{
    MaterialPaths materialPaths;
    materialPaths.albedoTexture = getTexturePath(scene, material, aiTextureType_DIFFUSE, "albedo");
    materialPaths.normalMapTexture = getTexturePath(scene, material, aiTextureType_NORMALS, "normal");
    materialPaths.metallicsTexture = getTexturePath(scene, material, aiTextureType_METALNESS, "matallic"); // Requires Assimp 5.0+
    materialPaths.roughnessTexture = getTexturePath(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, "roughness");
    materialPaths.ambientOcclusionTexture = getTexturePath(scene, material, aiTextureType_LIGHTMAP, "ao");
    materialPaths.emissiveTexture = getTexturePath(scene, material, aiTextureType_EMISSIVE, "emissive");

    return materialPaths;
}

fs::path AssimpModelLoader::saveEmbeddedTexture(const aiTexture *texture, const std::string &materialParam)
{
    // Determine the file extension based on the desired format
    std::string extension = ".png"; // Default to PNG
    if (texture->achFormatHint[0] != '\0')
    {
        std::string hint(texture->achFormatHint);
        if (hint == "jpg" || hint == "jpeg") extension = ".jpg";
    }

    // Create a filename using the material parameter
    std::string textureName = m_path.stem().string() + "_" + materialParam + extension;
    fs::path texturePath = m_path.parent_path() / textureName;

    if (texture->mHeight == 0)
    {
        // Handle compressed texture data
        int width, height, channels;
        unsigned char *data = stbi_load_from_memory(reinterpret_cast<const unsigned char *>(texture->pcData),
                                                    texture->mWidth, &width, &height, &channels, 4);
        if (data)
        {
            // Save as PNG or JPG
            if (extension == ".png")
                stbi_write_png(texturePath.string().c_str(), width, height, 4, data, width * 4);
            else if (extension == ".jpg")
                stbi_write_jpg(texturePath.string().c_str(), width, height, 4, data, 90); // Quality = 90

            stbi_image_free(data);
        }
        else
        {
            SKY_CORE_ERROR("Failed to decode compressed texture for {0}", materialParam);
        }
    }
    else
    {
        // Handle uncompressed texture data (e.g., RGBA8888)
        SKY_CORE_ERROR("Uncompressed texture data is not supported in this implementation.");
    }

    return fs::relative(texturePath, ProjectManager::getConfig().getAssetDirectory());
}
} // namespace sky