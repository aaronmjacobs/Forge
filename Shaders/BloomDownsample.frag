#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "RenderQuality.glsl"

layout(constant_id = 0) const int kRenderQuality = kRenderQuality_High;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

const vec2 kMediumQualityOffsets[5] =
{
   vec2(0, 0),

   vec2(-1, -1),
   vec2(-1, 1),
   vec2(1, -1),
   vec2(1, 1)
};

const float kMediumQualityWeights[5] =
{
   0.2,

   0.2,
   0.2,
   0.2,
   0.2
};

const vec2 kHighQualityOffsets[13] =
{
   vec2(0, 0),

   vec2(-1, -1),
   vec2(-1, 1),
   vec2(1, -1),
   vec2(1, 1),

   vec2(0, -2),
   vec2(0, 2),
   vec2(-2, 0),
   vec2(2, 0),

   vec2(-2, -2),
   vec2(-2, 2),
   vec2(2, -2),
   vec2(2, 2)
};

const float kHighQualityWeights[13] =
{
   0.125,

   0.125,
   0.125,
   0.125,
   0.125,

   0.0625,
   0.0625,
   0.0625,
   0.0625,

   0.03125,
   0.03125,
   0.03125,
   0.03125
};

void main()
{
   vec2 texelSize = 1.0 / textureSize(inputTexture, 0);

   outColor = vec4(0.0);
   if (kRenderQuality == kRenderQuality_Low)
   {
      outColor = texture(inputTexture, inTexCoord);
   }
   else if (kRenderQuality == kRenderQuality_Medium)
   {
      for (int i = 0; i < 5; ++i)
      {
         vec2 uv = inTexCoord + kMediumQualityOffsets[i] * texelSize;
         outColor += texture(inputTexture, uv) * kMediumQualityWeights[i];
      }
   }
   else if (kRenderQuality == kRenderQuality_High)
   {
      for (int i = 0; i < 13; ++i)
      {
         vec2 uv = inTexCoord + kHighQualityOffsets[i] * texelSize;
         outColor += texture(inputTexture, uv) * kHighQualityWeights[i];
      }
   }
}
