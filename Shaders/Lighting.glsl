#if !defined(LIGHTING_GLSL)
#define LIGHTING_GLSL

#include "View.glsl"

struct DirectionalLight
{
   vec4 color;
   vec4 direction;
};

struct PointLight
{
   vec4 colorRadius;
   vec4 position;
   vec2 nearFar;
   int shadowMapIndex;
};

struct SpotLight
{
   mat4 worldToShadow;
   vec4 colorRadius;
   vec4 positionBeamAngle;
   vec4 directionCutoffAngle;
   int shadowMapIndex;
};

struct LightingParams
{
   vec3 diffuseColor;
   vec3 specularColor;
   float shininess;

   vec3 surfacePosition;
   vec3 surfaceNormal;

   vec3 cameraPosition;
};

float vec3Max(vec3 vec)
{
   return max(vec.x, max(vec.y, vec.z));
}

float normalizeDepth(float linearDepth, float near, float far)
{
   float depthNDC = ((near + far) - (2.0 * near * far) / linearDepth) / (far - near);
   return (depthNDC + 1.0) * 0.5;
}

float calcAttenuation(vec3 toLight, float radius)
{
   float squaredDist = dot(toLight, toLight);
   float squaredFalloff = (1.0 / (1.0 + squaredDist));

   float dist = sqrt(squaredDist);
   float linearFalloff = 1.0 - clamp(dist / radius, 0.0, 1.0);

   return squaredFalloff * linearFalloff;
}

vec3 calcAmbient(vec3 lightColor, vec3 diffuseColor)
{
   return lightColor * diffuseColor * 0.05;
}

vec3 calcDiffuse(vec3 lightColor, vec3 diffuseColor, vec3 surfaceNormal, vec3 toLightDirection)
{
   float diffuseAmount = max(0.0, dot(surfaceNormal, toLightDirection));

   return lightColor * (diffuseColor * diffuseAmount) * 0.95;
}

vec3 calcSpecular(vec3 lightColor, vec3 specularColor, float shininess, vec3 surfacePosition, vec3 surfaceNormal, vec3 toLightDirection, vec3 cameraPosition)
{
   vec3 toCamera = normalize(cameraPosition - surfacePosition);
   vec3 reflection = reflect(-toLightDirection, surfaceNormal);

   float specularBase = max(0.0, dot(toCamera, reflection));
   float specularAmount = pow(specularBase, max(1.0, shininess));

   return lightColor * (specularColor * specularAmount);
}

float calcVisibility(samplerCubeArrayShadow shadowMaps, int shadowMapIndex, vec2 nearFar, vec3 toLight, vec3 toLightDirection)
{
   float computedDistance = length(toLight);

   return texture(shadowMaps, vec4(-toLightDirection, shadowMapIndex), computedDistance);
}

float calcVisibility(sampler2DArrayShadow shadowMaps, int shadowMapIndex, vec3 surfacePosition, mat4 worldToShadow)
{
   float visibility = 0.0;

   vec4 shadowPosition = worldToShadow * vec4(surfacePosition, 1.0);
   vec2 shadowTexCoords = (shadowPosition.xy / shadowPosition.w) * 0.5 + 0.5;
   float shadowDepth = shadowPosition.z / shadowPosition.w;

   vec2 texelSize = 1.0 / textureSize(shadowMaps, 0).xy;
   for (int x = -1; x <= 1; ++x)
   {
      for (int y = -1; y <= 1; ++y)
      {
         vec2 offset = vec2(x, y) * texelSize;
         visibility += texture(shadowMaps, vec4(shadowTexCoords + offset, shadowMapIndex, shadowDepth));
      }
   }

   return visibility / 9.0;
}

vec3 calcDirectionalLighting(DirectionalLight directionalLight, LightingParams lightingParams)
{
   vec3 lightColor = directionalLight.color.rgb;
   vec3 lightDirection = directionalLight.direction.xyz;

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, -lightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, -lightDirection, lightingParams.cameraPosition);

   return ambient + (diffuse + specular);
}

vec3 calcPointLighting(PointLight pointLight, LightingParams lightingParams, samplerCubeArrayShadow shadowMaps)
{
   vec3 lightColor = pointLight.colorRadius.rgb;
   vec3 lightPosition = pointLight.position.xyz;
   float lightRadius = pointLight.colorRadius.w;

   vec3 toLight = lightPosition - lightingParams.surfacePosition;
   vec3 toLightDirection = normalize(toLight);

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, toLightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, toLightDirection, lightingParams.cameraPosition);

   float attenuation = calcAttenuation(toLight, lightRadius);

   float brightness = attenuation;
   vec3 directLighting = (diffuse + specular) * brightness;
   vec3 indirectLighting = ambient * brightness;

   if (vec3Max(directLighting) > 0.0 && pointLight.shadowMapIndex >= 0)
   {
      float visibility = calcVisibility(shadowMaps, pointLight.shadowMapIndex, pointLight.nearFar, toLight, toLightDirection);
      directLighting *= visibility;
   }

   return directLighting + indirectLighting;
}

vec3 calcSpotLighting(SpotLight spotLight, LightingParams lightingParams, sampler2DArrayShadow shadowMaps)
{
   vec3 lightColor = spotLight.colorRadius.rgb;
   vec3 lightPosition = spotLight.positionBeamAngle.xyz;
   vec3 lightDirection = spotLight.directionCutoffAngle.xyz;
   float lightRadius = spotLight.colorRadius.w;
   float lightBeamAngle = spotLight.positionBeamAngle.w;
   float lightCutoffAngle = spotLight.directionCutoffAngle.w;

   vec3 toLight = lightPosition - lightingParams.surfacePosition;
   vec3 toLightDirection = normalize(toLight);

   vec3 ambient = calcAmbient(lightColor, lightingParams.diffuseColor);
   vec3 diffuse = calcDiffuse(lightColor, lightingParams.diffuseColor, lightingParams.surfaceNormal, toLightDirection);
   vec3 specular = calcSpecular(lightColor, lightingParams.specularColor, lightingParams.shininess, lightingParams.surfacePosition, lightingParams.surfaceNormal, toLightDirection, lightingParams.cameraPosition);

   float attenuation = calcAttenuation(toLight, lightRadius);

   float spotAngle = acos(dot(lightDirection, -toLightDirection));
   float clampedAngle = clamp(spotAngle, lightBeamAngle, lightCutoffAngle);
   float spotMultiplier = 1.0 - ((clampedAngle - lightBeamAngle) / (lightCutoffAngle - lightBeamAngle));

   float brightness = attenuation * spotMultiplier;
   vec3 directLighting = (diffuse + specular) * brightness;
   vec3 indirectLighting = ambient * brightness;

   if (vec3Max(directLighting) > 0.0 && spotLight.shadowMapIndex >= 0)
   {
      float visibility = calcVisibility(shadowMaps, spotLight.shadowMapIndex, lightingParams.surfacePosition, spotLight.worldToShadow);
      directLighting *= visibility;
   }

   return directLighting + indirectLighting;
}

#endif
