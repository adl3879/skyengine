#include "light.h"

namespace sky
{
static LightType getLightTypeFromString(const std::string &type) 
{
    if (type == "directionalLight") return LightType::Directional;
    if (type == "pointLight") return LightType::Point;
    if (type == "spotLight") return LightType::Spot;
    return LightType::None;
}

static const std::string &getStringFromLightType(LightType type) 
{
    static const std::unordered_map<LightType, std::string> lightTypeToString = {
        {LightType::None, "none"},
        {LightType::Directional, "directionalLight"},
        {LightType::Point, "pointLight"},
        {LightType::Spot, "spotLight"},
    };
    return lightTypeToString.at(type);
}

void Light::setConeAngles(float innerConeAngle, float outerConeAngle)
{
	// See KHR_lights_punctual spec - formulas are taken from it
	scaleOffset.x = 1.f / std::max(0.001f, std::cos(innerConeAngle) - std::cos(outerConeAngle));
	scaleOffset.y = -std::cos(outerConeAngle) * scaleOffset.x;
}

int Light::getShaderType() const
{
	// see light.glsl - should be the same!
	constexpr int TYPE_DIRECTIONAL_LIGHT = 0;
	constexpr int TYPE_POINT_LIGHT = 1;
	constexpr int TYPE_SPOT_LIGHT = 2;
	switch (type)
	{
		case LightType::Directional: return TYPE_DIRECTIONAL_LIGHT;
		case LightType::Point: return TYPE_POINT_LIGHT;
		case LightType::Spot: return TYPE_SPOT_LIGHT;
		default: assert(false);
	}
	return 0;
}

void Light::serialize(YAML::Emitter &out) 
{
	out << getStringFromLightType(type) << YAML::BeginMap;
	out << YAML::Key << "color" << YAML::Value << YAML::Flow << YAML::BeginSeq << color.r << color.g << color.b
		<< YAML::EndSeq;
	out << YAML::Key << "range" << YAML::Value << range;
	out << YAML::Key << "intensity" << YAML::Value << intensity;
	out << YAML::Key << "scaleOffset" << YAML::Value << YAML::Flow << YAML::BeginSeq << scaleOffset.x
		<< scaleOffset.y << YAML::EndSeq;
	out << YAML::Key << "castShadow" << YAML::Value << castShadow;
	out << YAML::EndMap;
}
void Light::deserialize(YAML::detail::iterator_value entity) 
{
	auto light = entity[getStringFromLightType(type)];
	if (light["color"])
	{
		auto colorSeq = light["color"].as<std::vector<float>>();
		if (colorSeq.size() == 3) color = {colorSeq[0], colorSeq[1], colorSeq[2]};
	}
	if (light["range"]) range = light["range"].as<float>();
	if (light["intensity"]) intensity = light["intensity"].as<float>(); 
	if (light["scaleOffset"])
	{
		auto scaleOffsetSeq = light["scaleOffset"].as<std::vector<float>>();
		if (scaleOffsetSeq.size() == 2)
		{
			scaleOffset = {scaleOffsetSeq[0], scaleOffsetSeq[1]};
		}
	}
	if (light["castShadow"]) castShadow = light["castShadow"].as<bool>();
}
}