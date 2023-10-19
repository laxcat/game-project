$input v_texcoord0, v_norm, v_pos

#include <bgfx_shader.sh>
#include "../../shared_defines.h"

// SHADER UTILS

float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

// not sure about this attenuation function... could at least have better cutoff parameter
// https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
float calcAttenuation(float lightRadius, vec3 lightPos, vec3 fragPos, vec3 fragNormal, float cutoff) {
    // calculate normalized light vector and distance to sphere light surface
    vec3 L = lightPos - fragPos;
    float distance = length(L);
    float d = max(distance - lightRadius, 0.0);
    L /= distance;

    // calculate basic attenuation
    float denom = d / lightRadius + 1.0;
    float attenuation = 1.0 / (denom*denom);

    // scale and bias attenuation such that:
    //   attenuation == 0 at extent of max influence
    //   attenuation == 1 when d == 0
    attenuation = (attenuation - cutoff) / (1.0 - cutoff);
    attenuation = max(attenuation, 0.0);

    float dot = max(dot(L, fragNormal), 0.0);
    return dot * attenuation;
}

SAMPLER2D(s_color, TEXTURE_SLOT_COLOR);
SAMPLER2D(s_norm,  TEXTURE_SLOT_NORM);
SAMPLER2D(s_metal, TEXTURE_SLOT_METAL);

// material
uniform vec4 u_materialBaseColor;
uniform vec4 u_materialPBRValues;
#define materialRoughness (u_materialPBRValues.x)
#define materialMetallic (u_materialPBRValues.y)
#define materialSpecular (u_materialPBRValues.z)
#define materialBaseColorIntensity (u_materialPBRValues.w)

// lights
// directional
uniform vec4 u_dirLightDir[MAX_DIRECTIONAL_LIGHTS];
#define lightDir(INDEX) (u_dirLightDir[INDEX].xyz)
uniform vec4 u_dirLightStrength[MAX_DIRECTIONAL_LIGHTS]; // x = ambient, y = diffuse, z = specular, w = global-factor
#define dirAmbientStength(INDEX) (u_dirLightStrength[INDEX].x)
#define dirDiffuseStength(INDEX) (u_dirLightStrength[INDEX].y)
#define dirSpecularStength(INDEX) (u_dirLightStrength[INDEX].z)
#define dirLightStength(INDEX) (u_dirLightStrength[INDEX].w)
uniform vec4 u_dirLightColor[MAX_DIRECTIONAL_LIGHTS];
#define dirColor(INDEX) (u_dirLightColor[INDEX].xyz)
// point
uniform vec4 u_pointLightPos[MAX_POINT_LIGHTS];
#define lightPos(INDEX) (u_pointLightPos[INDEX].xyz)
#define lightRadius(INDEX) (u_pointLightPos[INDEX].w)
uniform vec4 u_pointLightStrength[MAX_POINT_LIGHTS]; // x = ambient, y = diffuse, z = specular, w = global-factor
#define pointAmbientStength(INDEX) (u_pointLightStrength[INDEX].x)
#define pointDiffuseStength(INDEX) (u_pointLightStrength[INDEX].y)
#define pointSpecularStength(INDEX) (u_pointLightStrength[INDEX].z)
#define pointLightStength(INDEX) (u_pointLightStrength[INDEX].w)
uniform vec4 u_pointLightColor[MAX_POINT_LIGHTS];
#define pointColor(INDEX) (u_pointLightColor[INDEX].xyz)

// other
uniform vec4 u_fog;
uniform vec4 u_bgColor;
uniform vec4 u_cameraPos;
#define cameraPos (u_cameraPos.xyz)
uniform vec4 u_lightExtra;
#define dirLightCount (u_lightExtra.x)
#define pointLightCount (u_lightExtra.y)
#define pointLightCutoff (u_lightExtra.z)

/*
Blinn-Phong, with simple approximations for roughness/metalic material settings.

Approximation of blinn-phong exponent based on material-roughness.
http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

Metallic factor simply reduces diffuse color.
*/

void main() {
    // ensure unit length. can get too long from varying or possibly even from source
    vec3 norm = normalize(v_norm) * texture2D(s_norm, v_texcoord0).xyz;

    vec4 roughnessMetallicSample = texture2D(s_metal, v_texcoord0);
    float roughness = materialRoughness * roughnessMetallicSample.g;
    float metallic = materialMetallic * roughnessMetallicSample.b;
    float phongExp = (2.0 / pow(1.0 - roughness, 4.0) - 2.0) + metallic * 100.0;

    vec3 ambient = vec3(0.0);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    int count = int(dirLightCount);
    for (int i = 0; i < count; ++i) {
        // ambient
        ambient += dirAmbientStength(i) * dirLightStength(i) * dirColor(i);
        // diffuse
        diffuse += max(dot(norm, lightDir(i)), 0.0) * dirDiffuseStength(i) * dirLightStength(i) * dirColor(i);
        // specular
        vec3 viewDir = normalize(cameraPos - v_pos);
        vec3 halfwayDir = normalize(lightDir(i) + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), phongExp);
        specular += spec * dirSpecularStength(i) * dirLightStength(i) * dirColor(i);
    }
    count = int(pointLightCount);
    for (int i = 0; i < count; ++i) {
        float atten = calcAttenuation(lightRadius(i), lightPos(i), v_pos, norm, pointLightCutoff);
        vec3 pointLightDir = normalize(lightPos(i) - v_pos);

        // ambient
        ambient += pointAmbientStength(i) * pointLightStength(i) * atten * pointColor(i);
        // diffuse
        diffuse += max(dot(norm, pointLightDir), 0.0) * pointDiffuseStength(i) * pointLightStength(i) * atten * pointColor(i);
        // specular
        vec3 viewDir = normalize(cameraPos - v_pos);
        vec3 halfwayDir = normalize(pointLightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), phongExp);
        specular += spec * pointSpecularStength(i) * pointLightStength(i) * atten * pointColor(i);
    }

    // light + object color
    vec4 objColor = texture2D(s_color, v_texcoord0) * u_materialBaseColor;
    vec4 color = vec4((ambient + diffuse * (1.0 - metallic)) * objColor.rgb + specular, objColor.a);

    // distance fog
    // not really distance, but just how far z is from origin: abs(v_pos.z)
    // u_fog[0] = minDistance
    // u_fog[1] = fadeDistance (min + fade = max distance)
    // u_fog[2] = amount
    float fogMaxD = u_fog[0] + u_fog[1];
    float fogMix = map(clamp(abs(v_pos.z), u_fog[0], fogMaxD), u_fog[0], fogMaxD, 0.0, 1.0) * u_fog[2];
    color = mix(color, u_bgColor, fogMix);

    gl_FragColor = color;
}
