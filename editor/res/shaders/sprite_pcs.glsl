#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

#include "scene_data.glsl"

layout (push_constant) uniform constants
{
    SceneDataBuffer sceneData;
} pcs;

