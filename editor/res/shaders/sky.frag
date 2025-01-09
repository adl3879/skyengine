#version 460

#extension GL_GOOGLE_include_directive : enable

#include "sky_common.glsl"
#include "bindless.glsl"

layout(set = 0, binding = 1) uniform sampler2D transmittanceLUT;
layout(set = 0, binding = 2) uniform sampler2D skyLUT;

layout(location = 0) in vec2 outUV;

layout(location = 0) out vec4 outColor;

// layout (push_constant) uniform constants
// {
// } pcs;

vec3 getValFromSkyLUT(vec3 rayDir, vec3 sunDir) {
    float height = length(view.viewPos);
    vec3 up = view.viewPos / height;
    
    float horizonAngle = safeacos(sqrt(height * height - groundRadiusMM * groundRadiusMM) / height);
    float altitudeAngle = horizonAngle - acos(dot(rayDir, up));
    float azimuthAngle;
    
    if (abs(altitudeAngle) > (0.5*PI - .0001)) {
        azimuthAngle = 0.0;
    } else {
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);
        
        vec3 projectedDir = normalize(rayDir - up*(dot(rayDir, up)));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);
        azimuthAngle = atan(sinTheta, cosTheta) + PI;
    }
    
    float v = 0.5 + 0.5*sign(altitudeAngle)*sqrt(abs(altitudeAngle)*2.0/PI);
    vec2 uv = vec2(azimuthAngle / (2.0*PI), v);
    uv *= view.skyLUTRes;
    uv /= textureSize(skyLUT, 0);
    
    return texture(skyLUT, uv).rgb;
}

vec3 sunWithBloom(vec3 rayDir, vec3 sunDir) {
    const float sunSolidAngle = 0.53*PI/180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(rayDir, sunDir);
    if (cosTheta >= minSunCosTheta) return vec3(1.0);
    
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset*50000.0)*0.5;
    float invBloom = 1.0/(0.02 + offset*300.0)*0.01;
    return vec3(gaussianBloom+invBloom);
}

vec3 jodieReinhardTonemap(vec3 c) {
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.0);
    return mix(c / (l + 1.0), tc, tc);
}

void main() {
    vec3 sunDir = getSunDir(view.time);
    
    vec3 camDir = normalize(view.camDir.xyz);
    float camFOVWidth = PI/3.5;
    float camWidthScale = 2.0*tan(camFOVWidth/2.0);
    float camHeightScale = camWidthScale*view.resolution.y/view.resolution.x;
    
    vec3 camRight = normalize(cross(camDir, vec3(0.0, 1.0, 0.0)));
    vec3 camUp = normalize(cross(camRight, camDir));
    
    vec2 xy = 2.0 * outUV - 1.0;
    vec3 rayDir = normalize(camDir + camRight*xy.x*camWidthScale + camUp*xy.y*camHeightScale);
    
    vec3 lum = getValFromSkyLUT(rayDir, sunDir);

    vec3 sunLum = sunWithBloom(rayDir, sunDir);
    sunLum = smoothstep(0.002, 1.0, sunLum);
    
    if (length(sunLum) > 0.0) {
        if (rayIntersectSphere(view.viewPos, rayDir, groundRadiusMM) >= 0.0) {
            sunLum *= 0.0;
        } else {
            vec2 transmittanceUV = vec2(0.5 + 0.5*dot(normalize(view.viewPos), sunDir),
                                      max(0.0, min(1.0, (length(view.viewPos) - groundRadiusMM)/(atmosphereRadiusMM - groundRadiusMM))));
            sunLum *= texture(transmittanceLUT, transmittanceUV).rgb;
        }
    }
    lum += sunLum;
    
    // Tonemapping and gamma correction
    lum *= 20.0;
    lum = pow(lum, vec3(1.3));
    lum /= (smoothstep(0.0, 0.2, clamp(sunDir.y, 0.0, 1.0))*2.0 + 0.15);
    
    lum = jodieReinhardTonemap(lum);
    lum = pow(lum, vec3(1.0/2.2)); // Gamma correction
    
    outColor = vec4(lum, 1.0);

    // outColor = vec4(getValFromSkyLUT(rayDir, sunDir), 1.0);
}