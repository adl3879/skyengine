#version 450

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout (push_constant, scalar) uniform constants 
{
    SceneDataBuffer sceneData;
    uint cubemapImage;
} pcs;

layout(location = 0) out vec3 outTexCoord;

// Cube vertices
const vec3 positions[8] = vec3[8](
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0)
);

// Cube indices
const uint indices[36] = uint[36](
    // front
    0, 1, 2, 2, 3, 0,
    // right
    1, 5, 6, 6, 2, 1,
    // back
    5, 4, 7, 7, 6, 5,
    // left
    4, 0, 3, 3, 7, 4,
    // top
    3, 2, 6, 6, 7, 3,
    // bottom
    4, 5, 1, 1, 0, 4
);

void main() {
    // Get the vertex position from the cube
    uint vertexIndex = indices[gl_VertexIndex];
    vec3 position = positions[vertexIndex];
    
    // Remove translation from the view matrix to keep skybox centered on camera
    mat4 viewWithoutTranslation = mat4(
        pcs.sceneData.view[0],
        pcs.sceneData.view[1],
        pcs.sceneData.view[2],
        vec4(0.0, 0.0, 0.0, 1.0)
    );
    
    // The texture coordinates are the same as the vertex positions
    outTexCoord = position;
    
    // Transform the position
    vec4 pos = pcs.sceneData.proj * viewWithoutTranslation * vec4(position, 1.0);
    
    // Set z = w for maximum depth
    gl_Position = pos.xyww;
}