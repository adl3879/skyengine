#include "import_data.h"

#include <yaml-cpp/yaml.h>
#include "asset_management/asset.h"

namespace sky
{
bool ImportDataSerializer::serialize(const std::filesystem::path &path) 
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "id" << YAML::Value <<  m_data.id;
    out << YAML::Key << "type" << YAML::Value << std::string(assetTypeToString(m_data.type));
    out << YAML::Key << "source" << YAML::Value << m_data.source.string();
    out << YAML::Key << "destination" << YAML::Value << m_data.destination.string();
    out << YAML::Key << "version" << YAML::Value << m_data.version;
    out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();

    return true;
}

bool ImportDataSerializer::deserialize(const std::filesystem::path &path) 
{
    std::ifstream stream(path);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    if (!data["id"]) return false;

    m_data.id = data["id"].as<uint64_t>();
    m_data.type = assetTypeFromString(data["type"].as<std::string>());
    m_data.source = data["source"].as<std::string>();
    m_data.destination = data["destination"].as<std::string>();
    m_data.version = data["version"].as<std::string>();

    return true;
}
} // namespace sky