#include "environment_serializer.h"

#include "core/helpers/yaml.h"

namespace sky
{
bool EnvironmentSerializer::serialize(YAML::Emitter &out, const Environment &env)
{
    out << YAML::Key << "environment" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "skyboxHandle" << YAML::Value << env.skyboxHandle;
    out << YAML::EndMap;

    return true;
}

Environment EnvironmentSerializer::deserialize(YAML::Node data)
{
    Environment env{};
    
    if (data["environment"])
    {
        auto envNode = data["environment"];
        if (envNode["skyboxHandle"]) env.skyboxHandle = envNode["skyboxHandle"].as<UUID>();
    }

    return env;
}
}