#ifndef SCENE_DATA_GLSL
#define SCENE_DATA_GLSL

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_buffer_reference : require

#include "light.glsl"
#include "materials.glsl"

layout (buffer_reference, scalar) readonly buffer LightsDataBuffer {
    Light data[];
};

layout (buffer_reference, scalar) readonly buffer SceneDataBuffer {
    // camera
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec4 cameraPos;
    vec2 mousePos;
    vec2 viewportSize;

    // ambient
    vec3 ambientColor;
    float ambientIntensity;

    LightsDataBuffer lights;
    int numLights;
    int sunlightIndex; // if -1, there's no sun

    MaterialsBuffer materials;
} sceneDataBuffer;

#endif // SCENE_DATA_GLSL
