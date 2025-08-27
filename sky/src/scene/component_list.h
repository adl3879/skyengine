#pragma once

#include <skypch.h>

#include "components.h"

namespace sky
{
using ComponentList = std::tuple<
    IDComponent,
    TagComponent,
    TransformComponent,
    VisibilityComponent,
    DirectionalLightComponent,
    PointLightComponent,
    SpotLightComponent,
    ModelComponent,
    RelationshipComponent,
    SpriteRendererComponent,
    CameraComponent,
    RigidBodyComponent,
    BoxColliderComponent,
    CapsuleColliderComponent,
    SphereColliderComponent
>;
} // namespace sky