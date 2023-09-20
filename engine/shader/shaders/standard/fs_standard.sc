$input v_texcoord0, v_norm, v_pos

#include <bgfx_shader.sh>
#include "../../shared_defines.h"
#include "../../utils.sh"
#include "../../engine.h"


/*
Blinn-Phong, with simple approximations for roughness/metalic material settings.

Approximation of blinn-phong exponent based on material-roughness.
http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

Metallic factor simply reduces diffuse color.
*/

void main() {
    // ensure unit length. can get too long from varying or possibly even from source
    vec3 norm = normalize(v_norm);

    float phongExp = (2.0 / pow(materialRoughness, 4.0) - 2.0) + materialMetallic * 100.0;

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
    vec4 objColor = texture2D(s_texColor, v_texcoord0) * u_materialBaseColor;
    vec4 color = vec4((ambient + diffuse * (1.0 - materialMetallic)) * objColor.rgb + specular, objColor.a);

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
