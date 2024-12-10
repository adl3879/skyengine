#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "scene_data.glsl"
#include "lighting.glsl"
#include "mesh_pcs.glsl"

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;
layout (location = 4) in mat3 inTBN;

layout (location = 0) out vec4 outFragColor;

void main()
{
    MaterialData material = pcs.sceneData.materials.data[pcs.materialID];

    vec4 diffuse = sampleTexture2DLinear(material.diffuseTex, inUV);
    if (diffuse.a < 0.1) {
        discard;
    }

    vec3 baseColor = material.baseColor.rgb * diffuse.rgb;

    vec3 normal = normalize(inNormal).rgb;
    if (inTangent != vec4(0.0)) {
        // FIXME: sometimes Blender doesn't export tangents for some objects
        // for some reason. When we will start computing tangents manually,
        // this check can be removed
        normal = sampleTexture2DLinear(material.normalTex, inUV).rgb;
        // normal.y = 1 - normal.y; // flip to make OpenGL normal maps work
        normal = inTBN * normalize(normal * 2.0 - 1.0);
        normal = normalize(normal);
    }

    float metallicF = material.metallicRoughnessEmissive.r;
    float roughnessF = material.metallicRoughnessEmissive.g;

    float metallic = metallicF * sampleTexture2DLinear(material.metallicTex, inUV).r;
    float roughness = roughnessF * sampleTexture2DLinear(material.roughnessTex, inUV).r;

    // Optional: Convert roughness to linear space (if perceptual space)
    // roughness = roughness * roughness;

    roughness = max(roughness, 1e-2); // Prevent issues with zero roughness

    vec3 dielectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dielectricSpecular.r), black, metallic);
    vec3 f0 = mix(dielectricSpecular, baseColor, metallic);

    vec3 cameraPos = pcs.sceneData.cameraPos.xyz;
    vec3 fragPos = inPos.xyz;
    vec3 n = normal;
    vec3 v = normalize(cameraPos - fragPos);

    vec3 fragColor = vec3(0.0);
    for (int i = 0; i < pcs.sceneData.numLights; i++) {
        Light light = pcs.sceneData.lights.data[i];

        vec3 l = light.direction;
        if (light.type != TYPE_DIRECTIONAL_LIGHT) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        float occlusion = 1.0;
        // if (light.type == TYPE_DIRECTIONAL_LIGHT) {
        //     occlusion = calculateCSMOcclusion(
        //             fragPos, cameraPos, NoL,
        //             pcs.sceneData.csmShadowMapId, pcs.sceneData.cascadeFarPlaneZs, pcs.sceneData.csmLightSpaceTMs);
        // }

        fragColor += calculateLight(light, fragPos, n, v, l,
                diffuseColor, roughness, metallic, f0, occlusion);
    }

    // emissive
    float emissiveF = material.metallicRoughnessEmissive.b;
    vec3 emissiveColor = emissiveF * sampleTexture2DLinear(material.emissiveTex, inUV).rgb;
    fragColor += emissiveColor;

    // ambient
    fragColor += baseColor * pcs.sceneData.ambientColor * pcs.sceneData.ambientIntensity;

#if 0
    // CSM DEBUG
    uint cascadeIndex = chooseCascade(inPos, cameraPos, pcs.sceneData.cascadeFarPlaneZs);
    fragColor *= debugShadowsFactor(cascadeIndex);
#endif

#if 0
    // TANGENT DEBUG
    if (inTangent == vec4(0.0)) {
        fragColor = vec3(1.0f, 0.0f, 0.0f);
    }
#endif

#if 1
    // NORMAL DEBUG
	// fragColor = normal;
#endif

	outFragColor = vec4(fragColor, 1.0f);
    // outFragColor = vec4(1.0f, 0.f, 0.f, 1.f);
}