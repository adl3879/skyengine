#include "transform.h"

#include "core/helpers/yaml.h"

namespace sky
{
Transform::Transform() :
    m_position(0.0f, 0.0f, 0.0f), 
    m_rotationQuat(glm::identity<glm::quat>()), 
    m_scale(1.0f, 1.0f, 1.0f)
{}

void Transform::serialize(YAML::Emitter &out) 
{
	out << YAML::Key << "transform" << YAML::BeginMap;
    out << YAML::Key << "position"  << YAML::Value << m_position;
    out << YAML::Key << "rotation"  << YAML::Value << m_rotationQuat;
    out << YAML::Key << "scale"     << YAML::Value << m_scale;
	out << YAML::EndMap;
}

void Transform::deserialize(YAML::detail::iterator_value entity) 
{
    auto transformComponent = entity["transform"];
    m_position = transformComponent["position"].as<glm::vec3>();
    m_rotationQuat = transformComponent["rotation"].as<glm::quat>();
    m_scale = transformComponent["scale"].as<glm::vec3>();
}
} // namespace sky