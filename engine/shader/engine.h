SAMPLER2D(s_texColor,  0);

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
