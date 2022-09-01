#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

#include "Lighting.glsl"
#include "Masked.glsl"
#include "View.glsl"

layout(constant_id = 0) const bool kWithTextures = false;
layout(constant_id = 1) const bool kWithBlending = false;

layout(set = 1, binding = 0) uniform sampler2D normalBuffer;
layout(set = 1, binding = 1) uniform sampler2D ssaoBuffer;

layout(std430, set = 2, binding = 0) uniform Lights
{
   SpotLight spotLights[8];
   PointLight pointLights[8];
   DirectionalLight directionalLights[2];

   int numSpotLights;
   int numPointLights;
   int numDirectionalLights;
};
layout(set = 2, binding = 1) uniform samplerCubeArrayShadow pointLightShadowMaps;
layout(set = 2, binding = 2) uniform sampler2DArrayShadow spotLightShadowMaps;
layout(set = 2, binding = 3) uniform sampler2DArrayShadow directionalLightShadowMaps;

layout(set = 3, binding = 0) uniform sampler2D albedoTexture;
layout(set = 3, binding = 2) uniform sampler2D aoRoughnessMetalnessTexture;
layout(std430, set = 3, binding = 3) uniform Material
{
   vec4 albedo;
   vec4 emissive;

   float roughness;
   float metalness;
   float ambientOcclusion;
} material;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outRoughnessMetalness;

void main()
{
   SurfaceInfo surfaceInfo;
   surfaceInfo.albedo = material.albedo.rgb * inColor.rgb;
   surfaceInfo.roughness = material.roughness;
   surfaceInfo.metalness = material.metalness;
   surfaceInfo.ambientOcclusion = material.ambientOcclusion;

   float alpha = inColor.a * material.albedo.a;

   if (kWithTextures)
   {
      vec4 albedoSample = texture(albedoTexture, inTexCoord);
      surfaceInfo.albedo *= albedoSample.rgb;
      alpha *= albedoSample.a;

      vec4 aoRoughnessMetalnessSample = texture(aoRoughnessMetalnessTexture, inTexCoord);
      surfaceInfo.roughness = aoRoughnessMetalnessSample.g;
      surfaceInfo.metalness = aoRoughnessMetalnessSample.b;
      //surfaceInfo.ambientOcclusion = aoRoughnessMetalnessSample.r; // TODO Uncomment after fixing sponza textures
   }

   if (!kWithBlending && !passesMaskThreshold(alpha))
   {
      discard;
   }

   surfaceInfo.position = inPosition;

   vec2 screenTexCoord = gl_FragCoord.xy / textureSize(normalBuffer, 0);
   surfaceInfo.normal = texture(normalBuffer, screenTexCoord).rgb;
   surfaceInfo.ambientOcclusion *= texture(ssaoBuffer, screenTexCoord).r;

   if (!gl_FrontFacing)
   {
      surfaceInfo.normal = -surfaceInfo.normal;
   }

   vec3 lighting = vec3(0.0);

   for (int i = 0; i < numDirectionalLights; ++i)
   {
      lighting += calcDirectionalLighting(view.position.xyz, surfaceInfo, directionalLights[i], directionalLightShadowMaps);
   }

   for (int i = 0; i < numPointLights; ++i)
   {
      lighting += calcPointLighting(view.position.xyz, surfaceInfo, pointLights[i], pointLightShadowMaps);
   }

   for (int i = 0; i < numSpotLights; ++i)
   {
      lighting += calcSpotLighting(view.position.xyz, surfaceInfo, spotLights[i], spotLightShadowMaps);
   }

   outColor = vec4(lighting + material.emissive.rgb, alpha);
   outRoughnessMetalness = vec2(surfaceInfo.roughness, surfaceInfo.metalness);
}
