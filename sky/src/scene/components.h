#pragma once

#include <skypch.h>

#include "core/transform/transform.h"
#include "renderer/mesh.h"
#include "renderer/light.h"
#include "core/uuid.h"

namespace sky
{
using IDComponent = UUID;
using TagComponent = std::string;
using TransformComponent = Transform;
using VisibilityComponent = bool;

// Lights
struct DirectionalLightComponent { Light light; };
struct PointLightComponent { Light light; };
struct SpotLightComponent { Light light; };

struct ModelComponent
{
    ModelType type = ModelType::Custom;
    AssetHandle builtinMaterial = NULL_UUID;
    AssetHandle handle = NULL_UUID;
    std::map<uint32_t, AssetHandle> customMaterialOverrides;
};

struct RelationshipComponent
{
    UUID parent{NULL_UUID};
    UUID firstChild{NULL_UUID};
    UUID nextSibling{NULL_UUID};
    UUID previousSibling{NULL_UUID};   
};

struct SpriteRendererComponent
{
    AssetHandle textureHandle = NULL_UUID;
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
};
}