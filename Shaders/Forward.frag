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

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in mat3 inTBN;

layout(location = 0) out vec4 outColor;

void main()
{
   LightingParams lightingParams;

   float alpha = 1.0;
   if (kWithTextures)
   {
      vec4 diffuseSample = texture(diffuseTexture, inTexCoord);
      lightingParams.diffuseColor = inColor.rgb * diffuseSample.rgb;
      alpha = inColor.a * diffuseSample.a;

      vec3 tangentSpaceNormal = texture(normalTexture, inTexCoord).rgb * 2.0 - 1.0;
      lightingParams.surfaceNormal = normalize(inTBN * tangentSpaceNormal);
   }
   else
   {
      lightingParams.diffuseColor = inColor.rgb;
      alpha = inColor.a;

      lightingParams.surfaceNormal = normalize(inTBN[2]);
   }

   if (!kWithBlending && !passesMaskThreshold(alpha))
   {
      discard;
   }

   lightingParams.specularColor = vec3(0.5);
   lightingParams.shininess = 30.0;
   lightingParams.surfacePosition = inPosition;
   lightingParams.cameraPosition = view.position.xyz;

   vec3 color = vec3(0.0);

   for (int i = 0; i < numDirectionalLights; ++i)
   {
      color += calcDirectionalLighting(directionalLights[i], lightingParams, directionalLightShadowMaps);
   }

   for (int i = 0; i < numPointLights; ++i)
   {
      color += calcPointLighting(pointLights[i], lightingParams, pointLightShadowMaps);
   }

   for (int i = 0; i < numSpotLights; ++i)
   {
      color += calcSpotLighting(spotLights[i], lightingParams, spotLightShadowMaps);
   }

   outColor = vec4(color, alpha);
}
