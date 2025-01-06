#include "material_serializer.h"

#include <yaml-cpp/yaml.h>
#include "core/helpers/image.h"
#include "asset_management/asset_manager.h"

namespace sky
{
bool MaterialSerializer::serialize(const fs::path &path, const Material &mat) 
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << mat.name;
    out << YAML::Key << "baseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << mat.baseColor.r << mat.baseColor.g
        << mat.baseColor.b << mat.baseColor.a << YAML::EndSeq;
    out << YAML::Key << "metallicFactor" << YAML::Value << mat.metallicFactor;
    out << YAML::Key << "roughnessFactor" << YAML::Value << mat.roughnessFactor;
    out << YAML::Key << "ambientOcclusionFactor" << YAML::Value << mat.ambientOcclusionFactor;
    out << YAML::Key << "emissiveFactor" << YAML::Value << mat.emissiveFactor;
    out << YAML::Key << "albedoTexture" << YAML::Value << mat.albedoTextureHandle;
    out << YAML::Key << "normalMapTexture" << YAML::Value << mat.normalMapTextureHandle;
    out << YAML::Key << "metallicTexture" << YAML::Value << mat.metallicTextureHandle;
    out << YAML::Key << "roughnessTexture" << YAML::Value << mat.roughnessTextureHandle;
    out << YAML::Key << "ambientOcclusionTexture" << YAML::Value << mat.ambientOcclusionTextureHandle;
    out << YAML::Key << "emissiveTexture" << YAML::Value << mat.emissiveTextureHandle;
    out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();

    return true;
}

Material MaterialSerializer::deserialize(const fs::path &path) 
{
    std::ifstream stream(path);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    Material mat;

    if (data["name"])
    {
        mat.name = data["name"].as<std::string>();
    }

    if (data["baseColor"])
    {
        auto baseColor = data["baseColor"];
        mat.baseColor = LinearColor{baseColor[0].as<float>(), 
            baseColor[1].as<float>(), 
            baseColor[2].as<float>(),
            baseColor[3].as<float>()};
    }

    if (data["metallicFactor"]) mat.metallicFactor = data["metallicFactor"].as<float>();
    if (data["roughnessFactor"]) mat.roughnessFactor = data["roughnessFactor"].as<float>();
    if (data["ambientOcclusionFactor"]) mat.emissiveFactor = data["ambientOcclusionFactor"].as<float>();
    if (data["emissiveFactor"]) mat.emissiveFactor = data["emissiveFactor"].as<float>();

    if (data["albedoTexture"]) mat.albedoTextureHandle = data["albedoTexture"].as<AssetHandle>();
    if (data["normalMapTexture"]) mat.normalMapTextureHandle = data["normalMapTexture"].as<AssetHandle>();
    if (data["metallicTexture"]) mat.metallicTextureHandle = data["metallicTexture"].as<AssetHandle>();
    if (data["roughnessTexture"]) mat.roughnessTextureHandle = data["roughnessTexture"].as<AssetHandle>();
    if (data["ambientOcclusionTexture"]) mat.ambientOcclusionTextureHandle = data["ambientOcclusionTexture"].as<AssetHandle>();
    if (data["emissiveTexture"]) mat.emissiveTextureHandle = data["emissiveTexture"].as<AssetHandle>();

    const auto &albedoTex = AssetManager::getAsset<Texture2D>(mat.albedoTextureHandle);
    mat.albedoTexture = helper::loadImageFromTexture(albedoTex, VK_FORMAT_R8G8B8A8_SRGB);
    const auto &normalTex = AssetManager::getAsset<Texture2D>(mat.normalMapTextureHandle);
    mat.normalMapTexture = helper::loadImageFromTexture(normalTex, VK_FORMAT_R8G8B8A8_UNORM);
    const auto &metallicTex = AssetManager::getAsset<Texture2D>(mat.metallicTextureHandle);
    mat.metallicTexture = helper::loadImageFromTexture(metallicTex, VK_FORMAT_R8G8B8A8_UNORM);
    const auto &roughnessTex = AssetManager::getAsset<Texture2D>(mat.roughnessTextureHandle);
    mat.roughnessTexture = helper::loadImageFromTexture(roughnessTex, VK_FORMAT_R8G8B8A8_UNORM);
    const auto &ambientOcclusionTex = AssetManager::getAsset<Texture2D>(mat.ambientOcclusionTextureHandle);
    mat.ambientOcclusionTexture = helper::loadImageFromTexture(ambientOcclusionTex, VK_FORMAT_R8G8B8A8_UNORM);
    const auto &emissiveTex = AssetManager::getAsset<Texture2D>(mat.emissiveTextureHandle);
    mat.emissiveTexture = helper::loadImageFromTexture(emissiveTex, VK_FORMAT_R8G8B8A8_SRGB);

    return mat;
}
} // namespace sky