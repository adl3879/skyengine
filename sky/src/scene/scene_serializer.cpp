#include "scene_serializer.h"

#include "components.h"
#include "asset_management/asset_manager.h"
#include "scene.h"
#include "core/resource/environment_serializer.h"
#include "scene/components.h"
#include "yaml-cpp/emittermanip.h"

namespace sky
{
void SceneSerializer::serialize(const fs::path &path, AssetHandle handle) 
{
	YAML::Emitter out;
    out << YAML::BeginMap;

	out << YAML::Key << "name" << YAML::Value << m_scene->getName();
	out << YAML::Key << "type" << YAML::Value << sceneTypeToString(m_scene->getSceneType());
    EnvironmentSerializer envSerializer;
    envSerializer.serialize(out, m_scene->getEnvironment());

	out << YAML::Key << "entities" << YAML::Value << YAML::BeginSeq;
	for (auto eMap : m_scene->getEntityMap())
    {
        auto e = Entity{eMap.second, m_scene.get()};
        if (e.getComponent<TagComponent>() != "Root")
            serializeEntity(out, e, handle);
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

    EnvironmentSerializer envSerializer;
    auto env = envSerializer.deserialize(data);
    m_scene->setEnvironment(env);

	if (auto entities = data["entities"])
    {
		for (const auto &entity : entities)
        {
			const auto uuid = entity["uuid"].as<UUID>();
			const auto tag = entity["tag"].as<std::string>();
            auto dEntity = m_scene->createEntityWithUUID(uuid, tag);
            m_scene->getSceneGraph()->unlink(dEntity);
            deserializeEntity(entity, dEntity);

            auto rl = dEntity.getComponent<RelationshipComponent>();
            if (rl.parent == m_scene->getRootEntityUUID() && rl.previousSibling == NULL_UUID) 
            {
                // this is the first child of root
                auto root = m_scene->getRootEntity();
                root.getComponent<RelationshipComponent>().firstChild = dEntity.getComponent<IDComponent>();
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
		auto &tComponent = entity.getComponent<TransformComponent>().transform;
		tComponent.serialize(out);
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
    if (entity.hasComponent<CameraComponent>())
    {
        const auto &camComp = entity.getComponent<CameraComponent>();
        out << YAML::Key << "camera" << YAML::BeginMap;
        out << YAML::Key << "isPrimary" << YAML::Value << camComp.isPrimary;
        out << YAML::Key << "isActive" << YAML::Value << camComp.isActive;
        out << YAML::Key << "renderOrder" << YAML::Value << camComp.renderOrder;
        out << YAML::Key << "clearFlags" << YAML::Value << clearFlagsToString(camComp.camera.getClearFlags());
        out << YAML::Key << "backgroundColor" << YAML::Value << camComp.camera.getBackgroundColor();
        out << YAML::Key << "projectionType" << YAML::Value << projectionTypeToString(camComp.camera.getProjectionType());
        out << YAML::Key << "fieldOfView" << YAML::Value << camComp.camera.getFieldOfView();
        out << YAML::Key << "orthographicSize" << YAML::Value << camComp.camera.getOrthographicSize();
        out << YAML::Key << "nearPlane" << YAML::Value << camComp.camera.getNear();
        out << YAML::Key << "farPlane" << YAML::Value << camComp.camera.getFar();
        out << YAML::Key << "viewport" << YAML::Value << camComp.camera.getViewport();
        out << YAML::EndMap;
    }

    if (entity.hasComponent<RigidBodyComponent>())
    {
        out << YAML::Key << "rigidBody";
        out << YAML::BeginMap;

        const auto &rb = entity.getComponent<RigidBodyComponent>();
        out << YAML::Key << "mass" << YAML::Value << rb.Mass;
        out << YAML::Key << "motionType" << YAML::Value << physics::motionTypeToString(rb.MotionType);
        out << YAML::Key << "linearDamping" << YAML::Value << rb.LinearDamping;
        out << YAML::Key << "angularDamping" << YAML::Value << rb.AngularDamping;
        out << YAML::Key << "isKinematic" << YAML::Value << rb.IsKinematic;
        out << YAML::Key << "useGravity" << YAML::Value << rb.UseGravity;

        out << YAML::EndMap;
    }

    if (entity.hasComponent<BoxColliderComponent>())
    {
        out << YAML::Key << "boxCollider";
        out << YAML::BeginMap;

        const auto &bc = entity.getComponent<BoxColliderComponent>();
        out << YAML::Key << "size" << YAML::Value << bc.Size;
        out << YAML::Key << "isTrigger" << YAML::Value << bc.IsTrigger;

        out << YAML::EndMap;
    }

    if (entity.hasComponent<SphereColliderComponent>())
    {
        out << YAML::Key << "sphereCollider";
        out << YAML::BeginMap;

        const auto &sc = entity.getComponent<SphereColliderComponent>();
        out << YAML::Key << "radius" << YAML::Value << sc.Radius;
        out << YAML::Key << "isTrigger" << YAML::Value << sc.IsTrigger;

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
		auto &tComponent = entity.getComponent<TransformComponent>().transform;
		tComponent.deserialize(key);
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
    auto transform = entity.getComponent<TransformComponent>().transform;
	if (auto dl = key["directionalLight"])
    {
		auto &dlComponent = entity.addComponent<DirectionalLightComponent>();
		dlComponent.light.type = LightType::Directional;
		dlComponent.light.deserialize(key);
    }
	if (auto pl = key["pointLight"])
    {
		auto &plComponent = entity.addComponent<PointLightComponent>();
		plComponent.light.type = LightType::Point;
		plComponent.light.deserialize(key);
    }
	if (auto sl = key["spotLight"])
    {
		auto &slComponent = entity.addComponent<SpotLightComponent>();
		slComponent.light.type = LightType::Spot;
		slComponent.light.deserialize(key);
    }
	if (auto sprite = key["spriteRenderer"])
    {
        auto &spriteRenderer = entity.addComponent<SpriteRendererComponent>();
        spriteRenderer.tint = sprite["tint"].as<glm::vec4>();
        spriteRenderer.textureHandle = sprite["texture"].as<AssetHandle>();
    }
    if (auto camera = key["camera"])
    {
        auto &camComp = entity.addComponent<CameraComponent>();
        camComp.isPrimary = camera["isPrimary"].as<bool>();
        if (camComp.isPrimary)
        {
            m_scene->getCameraSystem()->setActiveCamera(entity);
        }
        camComp.isActive = camera["isActive"].as<bool>();
        camComp.renderOrder = camera["renderOrder"].as<uint32_t>();
        camComp.camera.setClearFlags(clearFlagsFromString(camera["clearFlags"].as<std::string>()));
        camComp.camera.setBackgroundColor(camera["backgroundColor"].as<glm::vec4>());
        camComp.camera.setProjectionType(projectionTypeFromString(camera["projectionType"].as<std::string>()));
        camComp.camera.setFieldOfView(camera["fieldOfView"].as<float>());
        camComp.camera.setOrthographicSize(camera["orthographicSize"].as<float>());
        camComp.camera.setNearClipPlane(camera["nearPlane"].as<float>());
        camComp.camera.setFarClipPlane(camera["farPlane"].as<float>());
        camComp.camera.setViewport(camera["viewport"].as<glm::vec4>());
    }

    if (auto rigidBody = key["rigidBody"])
    {
        auto &rb = entity.addComponent<RigidBodyComponent>();
        rb.Mass = rigidBody["mass"].as<float>();
        rb.MotionType = physics::motionTypeFromString(rigidBody["motionType"].as<std::string>());
        rb.LinearDamping = rigidBody["linearDamping"].as<float>();
        rb.AngularDamping = rigidBody["angularDamping"].as<float>();
        rb.IsKinematic = rigidBody["isKinematic"].as<bool>();
        rb.UseGravity = rigidBody["useGravity"].as<bool>();
    }

    if (auto boxCollider = key["boxCollider"])
    {
        auto &bc = entity.addComponent<BoxColliderComponent>();
        bc.Size = boxCollider["size"].as<glm::vec3>();
        bc.IsTrigger = boxCollider["isTrigger"].as<bool>();
    }

    if (auto sphereCollider = key["sphereCollider"])
    {
        auto &sc = entity.addComponent<SphereColliderComponent>();
        sc.Radius = sphereCollider["radius"].as<float>();
        sc.IsTrigger = sphereCollider["isTrigger"].as<bool>();
    }
}
} // namespace sky