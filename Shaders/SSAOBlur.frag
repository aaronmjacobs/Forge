#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(constant_id = 0) const bool kHorizontal = true;

layout(set = 1, binding = 0) uniform sampler2D ssaoTexture;
layout(set = 1, binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out float outBlurredSSAO;

vec3 getViewPosition(vec2 uv)
{
   float depth = texture(depthTexture, uv).r;
   float z = depth * 2.0 - 1.0;

   vec4 clipPosition = vec4(uv * 2.0 - 1.0, z, 1.0);
   vec4 viewPosition = view.clipToView * clipPosition;
   viewPosition /= viewPosition.w;

   return viewPosition.xyz;
}

float getDepth(vec2 uv)
{
   return getViewPosition(uv).z;
}

void main()
{
   float centerDepth = getDepth(inTexCoord);

   vec2 texelSize = 1.0 / textureSize(ssaoTexture, 0);

   float sum = 0.0;
   int numSamples = 0;
   for (int i = -2; i < 2; ++i)
   {
      vec2 offset = vec2(0.0, 0.0);
      if (kHorizontal)
      {
         offset.x = i * texelSize.x;
      }
      else
      {
         offset.y = i * texelSize.y;
      }
      vec2 uv = inTexCoord + offset;

      float offsetDepth = getDepth(uv);
      if (abs(offsetDepth - centerDepth) < 0.02)
      {
         sum += texture(ssaoTexture, uv).r;
         ++numSamples;
      }
   }

   outBlurredSSAO = numSamples > 0 ? sum / numSamples : 0.0;
}
