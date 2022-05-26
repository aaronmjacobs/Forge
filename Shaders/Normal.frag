#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "Masked.glsl"

layout(constant_id = 0) const bool kWithTextures = false;
layout(constant_id = 1) const bool kMasked = false;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in mat3 inTBN;

layout(location = 0) out vec4 outNormal;

void main()
{
   vec3 worldSpaceNormal = vec3(0.0);

   if (kWithTextures)
   {
      if (kMasked)
      {
         float alpha = texture(albedoTexture, inTexCoord).a;
         if (!passesMaskThreshold(alpha))
         {
            discard;
         }
      }

      vec3 tangentSpaceNormal = texture(normalTexture, inTexCoord).rgb * 2.0 - 1.0;
      tangentSpaceNormal.z = sqrt(clamp(1.0 - dot(tangentSpaceNormal.xy, tangentSpaceNormal.xy), 0.0, 1.0));
      tangentSpaceNormal = normalize(tangentSpaceNormal);
      worldSpaceNormal = normalize(inTBN * tangentSpaceNormal);
   }
   else
   {
      worldSpaceNormal = normalize(inTBN[2]);
   }

   outNormal = vec4(worldSpaceNormal, 0.0);
}
