#include "mesh_serializer.h"

#include "core/application.h"
#include "renderer/texture.h"
#include "asset_management/asset_manager.h"
#include "core/helpers/image.h"

namespace sky
{
static Material createMaterialFromPaths(MaterialPaths materialPaths, 
    AssetHandle handle, 
    const std::string &name)
{
    Material material;

    if (materialPaths.albedoTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.albedoTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.albedoTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_SRGB);
        material.albedoTextureHandle = textureHandle;
    }

    if (materialPaths.normalMapTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.normalMapTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.normalMapTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_UNORM);
        material.normalMapTextureHandle = textureHandle;
    }

    if (materialPaths.metallicsTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.metallicsTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.metallicTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_UNORM);
        material.metallicTextureHandle = textureHandle;
    }

    if (materialPaths.roughnessTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.roughnessTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.roughnessTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_UNORM);
        material.roughnessTextureHandle = textureHandle;
    }

    if (materialPaths.ambientOcclusionTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.ambientOcclusionTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.ambientOcclusionTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_UNORM);
        material.ambientOcclusionTextureHandle = textureHandle;
    }

    if (materialPaths.emissiveTexture != "")
    {
        auto textureHandle = AssetManager::getOrCreateAssetHandle(materialPaths.emissiveTexture, AssetType::Texture2D);
        AssetManager::addToDependencyList(handle, textureHandle);
        auto tex = AssetManager::getAsset<Texture2D>(textureHandle);
        material.emissiveTexture = helper::loadImageFromTexture(tex, VK_FORMAT_R8G8B8A8_SRGB);
        material.emissiveTextureHandle = textureHandle;
    }
    material.name = name.empty() ? "Unnamed" : name;

    return material;
}

bool MeshSerializer::serialize(const fs::path &path, std::vector<MeshLoaderReturn> meshes) 
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        SKY_CORE_ERROR("Failed to open mesh file: {0}", path.string());
        return false;
    }

    // Version
    uint16_t version = 0x0001U;
    file.write(reinterpret_cast<char *>(&version), sizeof(uint16_t));

    // Size
    uint16_t size = meshes.size();
    file.write(reinterpret_cast<char *>(&size), sizeof(uint16_t));

    for (auto &mesh : meshes)
    {
        // Name
        std::string name = mesh.mesh.name;
        auto nameLength = static_cast<uint32_t>(name.length());
        file.write(reinterpret_cast<char *>(&nameLength), sizeof(uint32_t));
        file.write(name.c_str(), nameLength);

        // Vertex data
        auto vertexCount = static_cast<uint32_t>(mesh.mesh.vertices.size());
        file.write(reinterpret_cast<char *>(&vertexCount), sizeof(uint32_t));
        file.write(reinterpret_cast<char *>(mesh.mesh.vertices.data()), sizeof(Vertex) * vertexCount);

        // Index data
        auto indexCount = static_cast<uint32_t>(mesh.mesh.indices.size());
        file.write(reinterpret_cast<char *>(&indexCount), sizeof(uint32_t));
        file.write(reinterpret_cast<char *>(mesh.mesh.indices.data()), sizeof(uint32_t) * indexCount);

        // Material data   
        // Material name
        std::string materialName = mesh.materialName;
        auto materialNameLength = static_cast<uint32_t>(materialName.length());
        file.write(reinterpret_cast<char *>(&materialNameLength), sizeof(uint32_t));
        file.write(materialName.c_str(), materialNameLength);

        // Albedo
        std::string albedo = mesh.materialPaths.albedoTexture.string();
        auto albedoLength = static_cast<uint32_t>(albedo.length());
        file.write(reinterpret_cast<char *>(&albedoLength), sizeof(uint32_t));
        file.write(albedo.c_str(), albedoLength);

        // Normal
        std::string normal = mesh.materialPaths.normalMapTexture.string();
        auto normalLength = static_cast<uint32_t>(normal.length());
        file.write(reinterpret_cast<char *>(&normalLength), sizeof(uint32_t));
        file.write(normal.c_str(), normalLength);

        // Metallic
        std::string metallic = mesh.materialPaths.metallicsTexture.string();
        auto metallicLength = static_cast<uint32_t>(metallic.length());
        file.write(reinterpret_cast<char *>(&metallicLength), sizeof(uint32_t));
        file.write(metallic.c_str(), metallicLength);

        // Roughness
        std::string roughness = mesh.materialPaths.roughnessTexture.string();
        auto roughnessLength = static_cast<uint32_t>(roughness.length());
        file.write(reinterpret_cast<char *>(&roughnessLength), sizeof(uint32_t));
        file.write(roughness.c_str(), roughnessLength);

        // AO
        std::string ao = mesh.materialPaths.ambientOcclusionTexture.string();
        auto aoLength = static_cast<uint32_t>(ao.length());
        file.write(reinterpret_cast<char *>(&aoLength), sizeof(uint32_t));
        file.write(ao.c_str(), aoLength);

		 // Emissive
        std::string emissive = mesh.materialPaths.emissiveTexture.string();
        auto emissiveLength = static_cast<uint32_t>(emissive.length());
        file.write(reinterpret_cast<char *>(&emissiveLength), sizeof(uint32_t));
        file.write(emissive.c_str(), emissiveLength);
    }

    file.close();
    return true;
}

std::vector<Mesh> MeshSerializer::deserialize(const fs::path &path, AssetHandle handle) 
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        SKY_CORE_ERROR("Failed to open mesh file: {0}", path.string());
        return {};
    }

    // Version
    uint16_t version;
    file.read(reinterpret_cast<char *>(&version), sizeof(uint16_t));

    // Size
    uint16_t size;
    file.read(reinterpret_cast<char *>(&size), sizeof(uint16_t));

    auto meshes = std::vector<Mesh>{};
    for (uint16_t i = 0; i < size; i++)
    {
        // Name
        uint32_t nameLength;
        file.read(reinterpret_cast<char *>(&nameLength), sizeof(uint32_t));
        std::string name(nameLength, '\0');
        file.read(name.data(), nameLength);

        // Vertex data
        uint32_t vertexCount;
        file.read(reinterpret_cast<char *>(&vertexCount), sizeof(uint32_t));
        std::vector<Vertex> vertices(vertexCount);
        file.read(reinterpret_cast<char *>(vertices.data()), sizeof(Vertex) * vertexCount);

        // Index data
        uint32_t indexCount;
        file.read(reinterpret_cast<char *>(&indexCount), sizeof(uint32_t));
        std::vector<uint32_t> indices(indexCount);
        file.read(reinterpret_cast<char *>(indices.data()), sizeof(uint32_t) * indexCount);

        // Material data
        // Name
        uint32_t materialNameLength;
        file.read(reinterpret_cast<char *>(&materialNameLength), sizeof(uint32_t));
        std::string materialName(materialNameLength, '\0');
        file.read(materialName.data(), materialNameLength);

        // Albedo
        uint32_t albedoLength;
        file.read(reinterpret_cast<char *>(&albedoLength), sizeof(uint32_t));
        std::string albedo(albedoLength, '\0');
        file.read(albedo.data(), albedoLength);

        // Normal
        uint32_t normalLength;
        file.read(reinterpret_cast<char *>(&normalLength), sizeof(uint32_t));
        std::string normal(normalLength, '\0');
        file.read(normal.data(), normalLength);

        // Metallic
        uint32_t metallicLength;
        file.read(reinterpret_cast<char *>(&metallicLength), sizeof(uint32_t));
        std::string metallic(metallicLength, '\0');
        file.read(metallic.data(), metallicLength);

        // Roughness
        uint32_t roughnessLength;
        file.read(reinterpret_cast<char *>(&roughnessLength), sizeof(uint32_t));
        std::string roughness(roughnessLength, '\0');
        file.read(roughness.data(), roughnessLength);

        // AO
        uint32_t aoLength;
        file.read(reinterpret_cast<char *>(&aoLength), sizeof(uint32_t));
        std::string ao(aoLength, '\0');
        file.read(ao.data(), aoLength);

		// Emissive
        uint32_t emissiveLength;
        file.read(reinterpret_cast<char *>(&emissiveLength), sizeof(uint32_t));
        std::string emissive(emissiveLength, '\0');
        file.read(emissive.data(), emissiveLength);

        auto material = createMaterialFromPaths({
			.albedoTexture = albedo,
			.normalMapTexture = normal,
			.metallicsTexture = metallic,
			.roughnessTexture = roughness,
			.ambientOcclusionTexture = ao,
			.emissiveTexture = emissive,
		}, handle, materialName);
        auto materialId = Application::getRenderer()->addMaterialToCache(material);

        auto mesh = Mesh{
            .vertices = vertices,
            .indices = indices,
            .material = materialId, 
			.name = name.empty() ? "Unnamed" : name,
        };
        meshes.push_back(mesh);
    }

    return meshes;
}
} // namespace sky