#pragma once

#include <skypch.h>

#include "core/transform/transform.h"
#include "renderer/scene_renderer.h"
#include "renderer/camera/camera.h"

namespace sky
{
using TagComponent = std::string;
using TransformComponent = Transform;
using MeshComponent = MeshId;
using CameraComponent = Camera;
}