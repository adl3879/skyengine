#pragma once

#include <skypch.h>

#include "core/transform/transform.h"
#include "renderer/scene_renderer.h"
#include "renderer/camera/perspective_camera.h"
#include "renderer/light.h"
#include "core/uuid.h"

namespace sky
{
using IDComponent = UUID;
using TagComponent = std::string;
using TransformComponent = Transform;
using CameraComponent = PerspectiveCamera;
using VisibilityComponent = bool;

// Lights
struct DirectionalLightComponent { Light light; };
struct PointLightComponent { Light light; };
struct SpotLightComponent { Light light; };

struct ModelComponent
{
    ModelType type = ModelType::Custom;
    AssetHandle handle = NULL_UUID;
};

struct HierarchyComponent
{
    UUID parent;
    std::vector<UUID> children;
};
}