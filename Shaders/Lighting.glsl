#if !defined(LIGHTING_GLSL)
#define LIGHTING_GLSL

#include "View.glsl"

struct DirectionalLight
{
   mat4 worldToShadow;
   vec4 color;
   vec4 direction;
   vec2 nearFar;
   int shadowMapIndex;
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

struct SurfaceInfo
{
   vec3 position;
   vec3 normal;

   vec3 albedo;
   float roughness;
   float metalness;
   float ambientOcclusion;
};

const float kPI = 3.14159265359;
const float kIndirectAmount = 0.03;

float positiveDot(vec3 first, vec3 second)
{
   return max(dot(first, second), 0.0);
}

float distributionGGX(vec3 normal, vec3 halfwayVector, float roughness)
{
   float alpha = roughness * roughness;
   float alphaSquared = alpha * alpha;
   float cosTheta = positiveDot(normal, halfwayVector);
   float cosThetaSquared = cosTheta * cosTheta;

   float numerator = alphaSquared;
   float denominator = (cosThetaSquared * (alphaSquared - 1.0) + 1.0);
   denominator = kPI * denominator * denominator;

   return numerator / denominator;
}

float geometrySchlickGGX(float cosTheta, float roughness)
{
   float r = (roughness + 1.0);
   float k = (r * r) / 8.0;

   float numerator = cosTheta;
   float denominator = cosTheta * (1.0 - k) + k;

   return numerator / denominator;
}

float geometrySmith(vec3 normal, vec3 viewDirection, vec3 lightDirection, float roughness)
{
   float cosThetaView = positiveDot(normal, viewDirection);
   float cosThetaLight = positiveDot(normal, lightDirection);
   float ggxView = geometrySchlickGGX(cosThetaView, roughness);
   float ggxLight = geometrySchlickGGX(cosThetaLight, roughness);

   return ggxView * ggxLight;
}

vec3 fresnelSchlick(float cosTheta, vec3 baseReflectivity)
{
   return baseReflectivity + (1.0 - baseReflectivity) * pow(1.0 - cosTheta, 5.0);
}

vec3 calcOutgoingRadiance(SurfaceInfo surfaceInfo, vec3 viewDirection, vec3 lightDirection, vec3 lightColor, float attenuation)
{
   vec3 halfwayVector = normalize(viewDirection + lightDirection);
   vec3 baseReflectivity = mix(vec3(0.04), surfaceInfo.albedo, surfaceInfo.metalness);

   float distribution = distributionGGX(surfaceInfo.normal, halfwayVector, surfaceInfo.roughness);
   float geometry = geometrySmith(surfaceInfo.normal, viewDirection, lightDirection, surfaceInfo.roughness);
   vec3 fresnel = fresnelSchlick(positiveDot(halfwayVector, viewDirection), baseReflectivity);

   vec3 specularContribution = fresnel;
   vec3 diffuseContribution = (vec3(1.0) - specularContribution) * (1.0 - surfaceInfo.metalness);
   vec3 diffuse = (surfaceInfo.albedo / kPI) * diffuseContribution;
   vec3 specular = (distribution * geometry * fresnel) / (4.0 * positiveDot(surfaceInfo.normal, viewDirection) * positiveDot(surfaceInfo.normal, lightDirection) + 0.0001);

   float nDotL = positiveDot(surfaceInfo.normal, lightDirection);

   return (diffuse + specular) * lightColor * attenuation * nDotL;
}

vec3 calcAmbient(vec3 lightColor, vec3 albedo, float ambientOcclusion, float attenuation)
{
   return lightColor * albedo * ambientOcclusion * attenuation;
}

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

float calcVisibility(samplerCubeArrayShadow shadowMaps, int shadowMapIndex, vec2 nearFar, vec3 toLight, vec3 toLightDirection)
{
   float computedDepth = vec3Max(abs(toLight));
   float normalizedDepth = normalizeDepth(computedDepth, nearFar.x, nearFar.y);

   return texture(shadowMaps, vec4(-toLightDirection, shadowMapIndex), normalizedDepth);
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

vec3 calcDirectionalLighting(vec3 viewPosition, SurfaceInfo surfaceInfo, DirectionalLight directionalLight, sampler2DArrayShadow shadowMaps)
{
   vec3 lightColor = directionalLight.color.rgb;
   vec3 lightDirection = -directionalLight.direction.xyz;
   vec3 viewDirection = normalize(viewPosition - surfaceInfo.position);

   vec3 directLighting = calcOutgoingRadiance(surfaceInfo, viewDirection, lightDirection, lightColor, 1.0);
   vec3 indirectLighting = calcAmbient(lightColor, surfaceInfo.albedo, surfaceInfo.ambientOcclusion, 1.0);

   if (vec3Max(directLighting) > 0.0 && directionalLight.shadowMapIndex >= 0)
   {
      float visibility = calcVisibility(shadowMaps, directionalLight.shadowMapIndex, surfaceInfo.position, directionalLight.worldToShadow);
      directLighting *= visibility;
   }

   return mix(directLighting, indirectLighting, kIndirectAmount);
}

vec3 calcPointLighting(vec3 viewPosition, SurfaceInfo surfaceInfo, PointLight pointLight, samplerCubeArrayShadow shadowMaps)
{
   vec3 lightColor = pointLight.colorRadius.rgb;
   vec3 lightPosition = pointLight.position.xyz;
   float lightRadius = pointLight.colorRadius.w;
   vec3 viewDirection = normalize(viewPosition - surfaceInfo.position);

   vec3 toLight = lightPosition - surfaceInfo.position;
   vec3 lightDirection = normalize(toLight);

   float attenuation = calcAttenuation(toLight, lightRadius);

   vec3 directLighting = calcOutgoingRadiance(surfaceInfo, viewDirection, lightDirection, lightColor, attenuation);
   vec3 indirectLighting = calcAmbient(lightColor, surfaceInfo.albedo, surfaceInfo.ambientOcclusion, attenuation);

   if (vec3Max(directLighting) > 0.0 && pointLight.shadowMapIndex >= 0)
   {
      float visibility = calcVisibility(shadowMaps, pointLight.shadowMapIndex, pointLight.nearFar, toLight, lightDirection);
      directLighting *= visibility;
   }

   return mix(directLighting, indirectLighting, kIndirectAmount);
}

vec3 calcSpotLighting(vec3 viewPosition, SurfaceInfo surfaceInfo, SpotLight spotLight, sampler2DArrayShadow shadowMaps)
{
   vec3 lightColor = spotLight.colorRadius.rgb;
   vec3 lightPosition = spotLight.positionBeamAngle.xyz;
   vec3 spotDirection = -spotLight.directionCutoffAngle.xyz;
   float lightRadius = spotLight.colorRadius.w;
   float lightBeamAngle = spotLight.positionBeamAngle.w;
   float lightCutoffAngle = spotLight.directionCutoffAngle.w;
   vec3 viewDirection = normalize(viewPosition - surfaceInfo.position);

   vec3 toLight = lightPosition - surfaceInfo.position;
   vec3 lightDirection = normalize(toLight);

   float spotAngle = acos(dot(lightDirection, spotDirection));
   float clampedAngle = clamp(spotAngle, lightBeamAngle, lightCutoffAngle);
   float spotMultiplier = 1.0 - ((clampedAngle - lightBeamAngle) / (lightCutoffAngle - lightBeamAngle));

   float attenuation = calcAttenuation(toLight, lightRadius) * spotMultiplier;

   vec3 directLighting = calcOutgoingRadiance(surfaceInfo, viewDirection, lightDirection, lightColor, attenuation);
   vec3 indirectLighting = calcAmbient(lightColor, surfaceInfo.albedo, surfaceInfo.ambientOcclusion, attenuation);

   if (vec3Max(directLighting) > 0.0 && spotLight.shadowMapIndex >= 0)
   {
      float visibility = calcVisibility(shadowMaps, spotLight.shadowMapIndex, surfaceInfo.position, spotLight.worldToShadow);
      directLighting *= visibility;
   }

   return mix(directLighting, indirectLighting, kIndirectAmount);
}

#endif
