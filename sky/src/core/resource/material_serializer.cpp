#include "material_serializer.h"

#include <yaml-cpp/yaml.h>

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
    out << YAML::Key << "emissiveFactor" << YAML::Value << mat.emissiveFactor;
    out << YAML::Key << "albedoTexture" << YAML::Value << mat.albedoTexture;
    out << YAML::Key << "normalMapTexture" << YAML::Value << mat.normalMapTexture;
    out << YAML::Key << "metallicTexture" << YAML::Value << mat.metallicTexture;
    out << YAML::Key << "roughnessTexture" << YAML::Value << mat.roughnessTexture;
    out << YAML::Key << "ambientOcclusionTexture" << YAML::Value << mat.ambientOcclusionTexture;
    out << YAML::Key << "emissiveTexture" << YAML::Value << mat.emissiveTexture;
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
        mat.baseColor = LinearColor{baseColor[0].as<float>(), baseColor[1].as<float>(), baseColor[2].as<float>(),
                                    baseColor[3].as<float>()};
    }

    if (data["metallicFactor"]) mat.metallicFactor = data["metallicFactor"].as<float>();
    if (data["roughnessFactor"]) mat.roughnessFactor = data["roughnessFactor"].as<float>();
    if (data["emissiveFactor"]) mat.emissiveFactor = data["emissiveFactor"].as<float>();
    if (data["albedoTexture"]) mat.albedoTexture = data["albedoTexture"].as<ImageID>();
    if (data["normalMapTexture"]) mat.normalMapTexture = data["normalMapTexture"].as<ImageID>();
    if (data["metallicTexture"]) mat.metallicTexture = data["metallicTexture"].as<ImageID>();
    if (data["roughnessTexture"]) mat.roughnessTexture = data["roughnessTexture"].as<ImageID>();
    if (data["ambientOcclusionTexture"]) mat.ambientOcclusionTexture = data["ambientOcclusionTexture"].as<ImageID>();
    if (data["emissiveTexture"]) mat.emissiveTexture = data["emissiveTexture"].as<ImageID>();

    return mat;
}
} // namespace sky