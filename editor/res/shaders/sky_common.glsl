#ifndef SKY_COMMON_GLSL
#define SKY_COMMON_GLSL

// Constants
const float PI = 3.14159265358;
const float groundRadiusMM = 6.360;
const float atmosphereRadiusMM = 6.460;
const vec3  groundAlbedo = vec3(0.3);
const vec3  rayleighScatteringBase = vec3(5.802, 19.558, 33.1);
const float rayleighAbsorptionBase = 0.0;
const float mieScatteringBase = 3.996;
const float mieAbsorptionBase = 4.4;
const vec3  ozoneAbsorptionBase = vec3(0.650, 1.881, .085);

// Uniform buffer object for view parameters
layout(set = 0, binding = 0) uniform ViewParams {
    vec3 viewPos;
    float time;
    vec2 resolution;
    vec2 tLUTRes;
    vec2 msLUTRes;
    vec2 skyLUTRes;
    vec4 camDir;
} view;

// Utility functions
float safeacos(float x) {
    return acos(clamp(x, -1.0, 1.0));
}

/*
 * Animates the sun movement.
 */
float getSunAltitude(float time)
{
    const float periodSec = 120.0;
    const float halfPeriod = periodSec / 2.0;
    const float sunriseShift = 0.1;
    float cyclePoint = (1.0 - abs((mod(time,periodSec)-halfPeriod)/halfPeriod));
    cyclePoint = (cyclePoint*(1.0+sunriseShift))-sunriseShift;
    return (0.5*PI)*cyclePoint;
}
vec3 getSunDir(float time)
{
    float altitude = getSunAltitude(time);
    return normalize(vec3(0.0, sin(altitude), -cos(altitude)));
}

float rayIntersectSphere(vec3 ro, vec3 rd, float rad) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - rad*rad;
    if (c > 0.0f && b > 0.0) return -1.0;
    float discr = b*b - c;
    if (discr < 0.0) return -1.0;
    if (discr > b*b) return (-b + sqrt(discr));
    return -b - sqrt(discr);
}

float getMiePhase(float cosTheta) {
    const float g = 0.8;
    const float scale = 3.0/(8.0*PI);
    float num = (1.0-g*g)*(1.0+cosTheta*cosTheta);
    float denom = (2.0+g*g)*pow((1.0 + g*g - 2.0*g*cosTheta), 1.5);
    return scale*num/denom;
}

float getRayleighPhase(float cosTheta) {
    const float k = 3.0/(16.0*PI);
    return k*(1.0+cosTheta*cosTheta);
}

void getScatteringValues(vec3 pos, 
    out vec3 rayleighScattering, 
    out float mieScattering,
    out vec3 extinction) 
{
    float altitudeKM = (length(pos)-groundRadiusMM)*1000.0;
    float rayleighDensity = exp(-altitudeKM/8.0);
    float mieDensity = exp(-altitudeKM/1.2);
    
    rayleighScattering = rayleighScatteringBase * rayleighDensity;
    float rayleighAbsorption = rayleighAbsorptionBase * rayleighDensity;
    
    mieScattering = mieScatteringBase * mieDensity;
    float mieAbsorption = mieAbsorptionBase * mieDensity;
    
    vec3 ozoneAbsorption = ozoneAbsorptionBase * max(0.0, 1.0 - abs(altitudeKM-25.0)/15.0);
    
    extinction = rayleighScattering + rayleighAbsorption + mieScattering + mieAbsorption + ozoneAbsorption;
}

#endif