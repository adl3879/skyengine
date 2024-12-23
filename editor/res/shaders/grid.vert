#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout (push_constant) uniform constants
{
    SceneDataBuffer sceneData;
} pcs;

layout (location = 0) out vec3 worldPos;

float gridSize = 150.0;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
	0, 1, 2, 2, 3, 0
);

void main()
{
	int idx = indices[gl_VertexIndex];
	vec3 position = pos[idx] * gridSize;

	gl_Position = pcs.sceneData.viewProj * vec4(position, 1.0);
    worldPos = position;
}