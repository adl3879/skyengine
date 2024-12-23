#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#extension GL_EXT_buffer_reference : require

struct MaterialData {
    vec4 baseColor;
    vec4 metallicRoughnessEmissive;
    uint diffuseTex;
    uint normalTex;
    uint metallicTex;
    uint roughnessTex;
    uint aoTex;
    uint emissiveTex;

    uint padding[2];
};

layout (buffer_reference, std430) readonly buffer MaterialsBuffer {
    MaterialData data[];
} materialsBuffer;

#endif // MATERIALS_GLSL
