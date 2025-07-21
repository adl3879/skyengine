#include "scene_serializer.h"

#include "components.h"
#include "asset_management/asset_manager.h"
#include "scene.h"
#include "yaml-cpp/emittermanip.h"

namespace sky
{
void SceneSerializer::serialize(const fs::path &path, AssetHandle handle) 
{
	YAML::Emitter out;
    out << YAML::BeginMap;

	out << YAML::Key << "name" << YAML::Value << m_scene->getName();
	out << YAML::Key << "type" << YAML::Value << sceneTypeToString(m_scene->getSceneType());
	
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

    if (data["name"]) m_scene->setName(data["name"].as<std::string>());
    if (data["type"]) m_scene->setSceneType(sceneTypeFromString(data["type"].as<std::string>()));

	if (auto entities = data["entities"])
    {
		for (const auto &entity : entities)
        {
			const auto uuid = entity["uuid"].as<UUID>();
			const auto tag = entity["tag"].as<std::string>();
			auto deserializedEntity = m_scene->createEntityWithUUID(uuid, tag);
            m_scene->getSceneGraph()->unlink(deserializedEntity);
			deserializeEntity(entity, deserializedEntity);

            if (uuid == m_scene->getRootEntityUUID()) 
            {
                auto root = m_scene->getRootEntity();
                auto &rl = root.getComponent<RelationshipComponent>();
                rl = deserializedEntity.getComponent<RelationshipComponent>();
            }
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
        const auto &rel = entity.getComponent<RelationshipComponent>();
        out << YAML::Key << "relationship" << YAML::BeginMap;
        out << YAML::Key << "parent" << YAML::Value << rel.parent;
        out << YAML::Key << "firstChild" << YAML::Value << rel.firstChild;
        out << YAML::Key << "nextSibling" << YAML::Value << rel.nextSibling;
        out << YAML::Key << "previousSibling" << YAML::Value << rel.previousSibling;
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
		out << YAML::Key << "handle" << YAML::Value << modelComponent.handle;
		out << YAML::Key << "type" << YAML::Value << modelTypeToString(modelComponent.type);
		out << YAML::Key << "builtinMaterial" << YAML::Value << modelComponent.builtinMaterial;
		out << YAML::Key << "customMaterialOverrides" << YAML::BeginMap;
		for (const auto &[index, handle] : modelComponent.customMaterialOverrides)
			out << YAML::Key << index << YAML::Value << handle;
		out << YAML::EndMap;
		out << YAML::EndMap;

		if (modelComponent.handle != NULL_UUID) 
			AssetManager::addToDependencyList(handle, modelComponent.handle);
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
	if (entity.hasComponent<SpriteRendererComponent>())
    {
		const auto &spriteRenderer = entity.getComponent<SpriteRendererComponent>();
        out << YAML::Key << "spriteRenderer" << YAML::BeginMap;
        out << YAML::Key << "tint" << YAML::Value << spriteRenderer.tint;
        out << YAML::Key << "texture" << YAML::Value << spriteRenderer.textureHandle;
        out << YAML::EndMap;
    }

	out << YAML::EndMap;
}

void SceneSerializer::deserializeEntity(YAML::detail::iterator_value key, Entity entity) 
{
	if (auto uuid = key["uuid"]) entity.getComponent<IDComponent>() = uuid.as<UUID>();
	if (auto tag = key["tag"]) entity.getComponent<TagComponent>() = tag.as<std::string>();
	if (auto visibility = key["visibility"]) entity.getComponent<VisibilityComponent>() = visibility.as<bool>();
	if (auto relationship = key["relationship"])
    {
        auto &rel = entity.getComponent<RelationshipComponent>();
        rel.parent = relationship["parent"].as<UUID>();
        rel.firstChild = relationship["firstChild"].as<UUID>();
        rel.nextSibling = relationship["nextSibling"].as<UUID>();
        rel.previousSibling = relationship["previousSibling"].as<UUID>();
    }
	if (auto transform = key["transform"])
    {
		auto &transformComponent = entity.getComponent<TransformComponent>();
		transformComponent.deserialize(key);
    }
	if (auto model = key["model"])
    {
		auto &modelComponent = entity.addComponent<ModelComponent>();
		modelComponent.type = stringToModelType(model["type"].as<std::string>());
		modelComponent.handle = model["handle"].as<UUID>();
		modelComponent.builtinMaterial = model["builtinMaterial"].as<UUID>();
		for (const auto &overrideNode : model["customMaterialOverrides"])
		{
			auto index = overrideNode.first.as<uint32_t>();
			auto handle = overrideNode.second.as<AssetHandle>();
			modelComponent.customMaterialOverrides[index] = handle;
		}
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
	if (auto sprite = key["spriteRenderer"])
    {
        auto &spriteRenderer = entity.addComponent<SpriteRendererComponent>();
        spriteRenderer.tint = sprite["tint"].as<glm::vec4>();
        spriteRenderer.textureHandle = sprite["texture"].as<AssetHandle>();
    }
}
} // namespace sky