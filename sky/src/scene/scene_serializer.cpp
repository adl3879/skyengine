#include "scene_serializer.h"

#include "core/helpers/yaml.h"
#include "components.h"
#include "asset_management/asset_manager.h"

namespace sky
{
void SceneSerializer::serialize(const fs::path &path, AssetHandle handle) 
{
	YAML::Emitter out;
    out << YAML::BeginMap;
	
	out << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;
	for (auto entityId : m_scene->getRegistry().view<entt::entity>())
    {
		Entity entity{entityId, m_scene.get()};
		serializeEntity(out, entity, handle);
    }

    out << YAML::EndSeq;
	out << YAML::EndMap;

    std::ofstream fout(path);
    fout << out.c_str();
}

bool SceneSerializer::deserialize(const fs::path &filepath) 
{ 
	std::ifstream stream(filepath);
    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
	if (auto entities = data["entities"])
    {
		for (const auto &entity : entities)
        {
			const auto uuid = entity["uuid"].as<UUID>();
			const auto tag = entity["tag"].as<std::string>();
			auto deserializedEntity = m_scene->createEntityWithUUID(uuid, tag);
			deserializeEntity(entity, deserializedEntity);
		}
    }

	return true;
}

void SceneSerializer::serializeEntity(YAML::Emitter &out, Entity entity, AssetHandle handle) 
{
	out << YAML::BeginMap;

	out << YAML::Key << "uuid" << YAML::Value << entity.getComponent<IDComponent>();
	out << YAML::Key << "tag" << YAML::Value << entity.getComponent<TagComponent>();
	out << YAML::Key << "visibility" << YAML::Value << entity.getComponent<VisibilityComponent>();
    {
		const auto &hierarchyComponent = entity.getComponent<HierarchyComponent>();
		out << YAML::Key << "hierarchy" << YAML::BeginMap;
		out << YAML::Key << "parent" << hierarchyComponent.parent;
		out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;
		for (const auto &child : hierarchyComponent.children) out << child;
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}
	if (entity.hasComponent<TransformComponent>())
    {
		auto &transformComponent = entity.getComponent<TransformComponent>();
		transformComponent.serialize(out);
	}
	if (entity.hasComponent<ModelComponent>())
    {
		const auto &modelComponent = entity.getComponent<ModelComponent>();
		out << YAML::Key << "model" << YAML::BeginMap;
		out << YAML::Key << "handle" << YAML::Key << modelComponent.handle;
		out << YAML::EndMap;

		AssetManager::addToDependecyList(handle, modelComponent.handle);
    }
	if (entity.hasComponent<DirectionalLightComponent>())
    {
		auto &dlComponent = entity.getComponent<DirectionalLightComponent>();
		dlComponent.light.serialize(out);
    }
	if (entity.hasComponent<PointLightComponent>())
    {
		auto &plComponent = entity.getComponent<PointLightComponent>();
		plComponent.light.serialize(out);
    }
	if (entity.hasComponent<SpotLightComponent>())
    {
		auto &slComponent = entity.getComponent<SpotLightComponent>();
		slComponent.light.serialize(out);
    }

	out << YAML::EndMap;
}

void SceneSerializer::deserializeEntity(YAML::detail::iterator_value key, Entity entity) 
{
	if (auto uuid = key["uuid"]) entity.getComponent<IDComponent>() = uuid.as<UUID>();
	if (auto tag = key["tag"]) entity.getComponent<TagComponent>() = tag.as<std::string>();
	if (auto visibility = key["visibility"]) entity.getComponent<VisibilityComponent>() = visibility.as<bool>();
	if (auto hierarchy = key["hierarchy"])
    {
		auto &hierarchyComponent = entity.getComponent<HierarchyComponent>();
		hierarchyComponent.parent = hierarchy["parent"].as<UUID>();
		for (const auto child : hierarchy["children"])
			hierarchyComponent.children.push_back(child.as<UUID>());
    }
	if (auto transform = key["transform"])
    {
		auto &transformComponent = entity.getComponent<TransformComponent>();
		transformComponent.deserialize(key);
    }
	if (auto model = key["model"])
    {
		auto &modelComponent = entity.addComponent<ModelComponent>();
		modelComponent.handle = model["handle"].as<UUID>();
    }
	if (auto dl = key["directionalLight"])
    {
		auto &dlComponent = entity.addComponent<DirectionalLightComponent>();
		dlComponent.light.type = LightType::Directional;
		dlComponent.light.deserialize(key);
		dlComponent.light.id = m_scene->addLightToCache(dlComponent.light, entity.getComponent<TransformComponent>());
    }
	if (auto pl = key["pointLight"])
    {
		auto &plComponent = entity.addComponent<PointLightComponent>();
		plComponent.light.type = LightType::Point;
		plComponent.light.deserialize(key);
		plComponent.light.id = m_scene->addLightToCache(plComponent.light, entity.getComponent<TransformComponent>());
    }
	if (auto sl = key["spotLight"])
    {
		auto &slComponent = entity.addComponent<SpotLightComponent>();
		slComponent.light.type = LightType::Spot;
		slComponent.light.deserialize(key);
		slComponent.light.id = m_scene->addLightToCache(slComponent.light, entity.getComponent<TransformComponent>());
    }
}
} // namespace sky