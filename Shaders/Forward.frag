#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

#include "Lighting.glsl"
#include "Masked.glsl"
#include "View.glsl"

layout(constant_id = 0) const bool kWithTextures = false;
layout(constant_id = 1) const bool kWithBlending = false;

layout(std430, set = 1, binding = 0) uniform Lights
{
   SpotLight spotLights[8];
   PointLight pointLights[8];
   DirectionalLight directionalLights[2];

   int numSpotLights;
   int numPointLights;
   int numDirectionalLights;
};
layout(set = 1, binding = 1) uniform samplerCubeArrayShadow pointLightShadowMaps;
layout(set = 1, binding = 2) uniform sampler2DArrayShadow spotLightShadowMaps;
layout(set = 1, binding = 3) uniform sampler2DArrayShadow directionalLightShadowMaps;

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;
layout(set = 2, binding = 2) uniform sampler2D aoRoughnessMetalnessTexture;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in mat3 inTBN;

layout(location = 0) out vec4 outColor;

void main()
{
   SurfaceInfo surfaceInfo;

   float alpha = 1.0;
   if (kWithTextures)
   {
      vec4 albedoSample = texture(albedoTexture, inTexCoord);
      surfaceInfo.albedo = inColor.rgb * albedoSample.rgb;
      alpha = inColor.a * albedoSample.a;

      vec3 tangentSpaceNormal = texture(normalTexture, inTexCoord).rgb * 2.0 - 1.0;
      tangentSpaceNormal.z = sqrt(clamp(1.0 - dot(tangentSpaceNormal.xy, tangentSpaceNormal.xy), 0.0, 1.0));
      tangentSpaceNormal = normalize(tangentSpaceNormal);
      surfaceInfo.normal = normalize(inTBN * tangentSpaceNormal);

      vec4 aoRoughnessMetalnessSample = texture(aoRoughnessMetalnessTexture, inTexCoord);
      surfaceInfo.roughness = aoRoughnessMetalnessSample.g;
      surfaceInfo.metalness = aoRoughnessMetalnessSample.b;
      surfaceInfo.ambientOcclusion = 1.0; // aoRoughnessMetalnessSample.r; // TODO Uncomment after fixing sponza textures
   }
   else
   {
      surfaceInfo.albedo = inColor.rgb;
      alpha = inColor.a;

      surfaceInfo.normal = normalize(inTBN[2]);
      surfaceInfo.roughness = 0.5;
      surfaceInfo.metalness = 0.0;
      surfaceInfo.ambientOcclusion = 1.0;
   }

   if (!kWithBlending && !passesMaskThreshold(alpha))
   {
      discard;
   }

   surfaceInfo.position = inPosition;

   if (!gl_FrontFacing)
   {
      surfaceInfo.normal = -surfaceInfo.normal;
   }

   vec3 color = vec3(0.0);

   for (int i = 0; i < numDirectionalLights; ++i)
   {
      color += calcDirectionalLighting(view.position.xyz, surfaceInfo, directionalLights[i], directionalLightShadowMaps);
   }

   for (int i = 0; i < numPointLights; ++i)
   {
      color += calcPointLighting(view.position.xyz, surfaceInfo, pointLights[i], pointLightShadowMaps);
   }

   for (int i = 0; i < numSpotLights; ++i)
   {
      color += calcSpotLighting(view.position.xyz, surfaceInfo, spotLights[i], spotLightShadowMaps);
   }

   outColor = vec4(color, alpha);
}
