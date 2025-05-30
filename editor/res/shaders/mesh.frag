#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "scene_data.glsl"
#include "lighting.glsl"
#include "mesh_pcs.glsl"

#define DEPTH_ARRAY_SCALE 512

layout(set = 1, binding = 0) buffer writeonly s_Write_t
{
    uint data[DEPTH_ARRAY_SCALE];
} s_Write;

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
        normal = sampleTexture2DLinear(material.normalTex, inUV).rgb;
        // normal.y = 1 - normal.y; // flip to make OpenGL normal maps work
        normal = inTBN * normalize(normal * 2.0 - 1.0);
        normal = normalize(normal);
    }

    float metallicF = material.metallicRoughnessEmissive.r;
    float roughnessF = material.metallicRoughnessEmissive.g;

    // Use texture if it exists, otherwise use the base value
    float metallic = (material.metallicTex > 0) ? 
        mix(metallicF, sampleTexture2DLinear(material.metallicTex, inUV).r, 0.5) : 
        metallicF;
        
    float roughness = (material.roughnessTex > 0) ? 
        mix(roughnessF, sampleTexture2DLinear(material.roughnessTex, inUV).r, 0.5) : 
        roughnessF;

    roughness = max(roughness, 1e-2); // Prevent issues with zero roughness

    vec3 dielectricSpecular = vec3(0.04);
    vec3 black = vec3(0.0);
    vec3 diffuseColor = mix(baseColor * (1.0 - dielectricSpecular.r), black, metallic);
    vec3 f0 = mix(dielectricSpecular, baseColor, metallic);

    vec3 cameraPos = pcs.sceneData.cameraPos.xyz;
    vec3 fragPos = inPos.xyz;
    vec3 n = normal;
    vec3 v = normalize(cameraPos - fragPos);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < pcs.sceneData.numLights; i++) {
        Light light = pcs.sceneData.lights.data[i];

        vec3 l = light.direction;
        if (light.type != TYPE_DIRECTIONAL_LIGHT) {
            l = normalize(light.position - fragPos);
        }
        float NoL = clamp(dot(n, l), 0.0, 1.0);

        float occlusion = 1.0;

        Lo += calculateLight(light, fragPos, n, v, l,
            diffuseColor, roughness, metallic, f0, occlusion);
    }

    vec3 R = reflect(-v, n);
    vec3 irradiance = sampleTextureCubeLinear(pcs.sceneData.irradianceMapId, n).rgb;

    const float MAX_LOD = 4.0;
    vec3 prefilteredColor = sampleCubeLod(pcs.sceneData.prefilterMapId, R, roughness * MAX_LOD).rgb;

    float NoV = max(dot(n, v), 0.0);
    vec2 brdf = sampleTexture2DLinear(pcs.sceneData.brdfLutId, vec2(NoV, roughness)).rg;

    vec3 F = f0 + (1.0 - f0) * pow(1.0 - NoV, 5.0);
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 diffuseIBL = irradiance * diffuseColor;

    // Option 1: Linear scaling with metallic (current implementation)
    float reflectionScale = metallicF;
    
    // Option 2: Sharper cutoff for dielectrics
    // float reflectionScale = pow(metallic, 2.0); // Quadratic scaling
    
    // Option 3: Complete cutoff for pure dielectrics
    // float reflectionScale = metallic < 0.01 ? 0.0 : metallic;

    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y) * reflectionScale;

    vec3 iblColor = (kD * diffuseIBL + specularIBL) * pcs.sceneData.ambientIntensity;

    // emissive
    float emissiveF = material.metallicRoughnessEmissive.b;
    vec3 emissiveColor = emissiveF * sampleTexture2DLinear(material.emissiveTex, inUV).rgb;

    vec3 fragColor = Lo + iblColor + emissiveColor;

    // get the depth and scale it up by
    // the total number of buckets in depth array
    uint zIndex = uint(gl_FragCoord.z * DEPTH_ARRAY_SCALE);

    if (length(pcs.sceneData.mousePos - gl_FragCoord.xy) < 1) {
        s_Write.data[zIndex] = pcs.uniqueId;
    }

	outFragColor = vec4(fragColor, 1.0f);
}