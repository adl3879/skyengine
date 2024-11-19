#pragma once

#include <skypch.h>

#include "core/transform/transform.h"
#include "renderer/scene_renderer.h"
#include "renderer/camera/perspective_camera.h"
#include "core/uuid.h"

namespace sky
{
using IDComponent = UUID;
using TagComponent = std::string;
using TransformComponent = Transform;
using CameraComponent = PerspectiveCamera;

struct MeshComponent
{
    MeshID meshID;
};
struct HierarchyComponent
{
    UUID parent;
    std::vector<UUID> children;
};
}