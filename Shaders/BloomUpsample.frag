#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "RenderQuality.glsl"

layout(constant_id = 0) const int kRenderQuality = kRenderQuality_Low;
layout(constant_id = 1) const bool kHorizontal = false;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;
layout(set = 0, binding = 1) uniform sampler2D previousTexture;
layout(set = 0, binding = 2) uniform BloomParams
{
   float filterRadius;
   float colorMix;
};

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

const float kLowQualityWeights[3] =    { 0.27901, 0.44198, 0.27901 };
const float kMediumQualityWeights[5] = { 0.06136, 0.24477, 0.38774, 0.24477, 0.06136 };
const float kHighQualityWeights[7] =   { 0.00598, 0.06063, 0.24184, 0.38310, 0.24184, 0.06063, 0.00598 };

void main()
{
   vec2 inputTextureSize = textureSize(inputTexture, 0);
   vec2 sampleScale = filterRadius / inputTextureSize;

   vec2 direction = vec2(0.0);
   if (kHorizontal)
   {
      direction.x = 1.0;
   }
   else
   {
      direction.y = 1.0;
   }

   outColor = vec4(0.0);
   if (kRenderQuality == kRenderQuality_Low)
   {
      for (int i = 0; i < 3; ++i)
      {
         int offset = i - 1;
         vec2 uv = inTexCoord + direction * sampleScale * offset;
         outColor += texture(inputTexture, uv) * kLowQualityWeights[i];
      }
   }
   else if (kRenderQuality == kRenderQuality_Medium)
   {
      for (int i = 0; i < 5; ++i)
      {
         int offset = i - 2;
         vec2 uv = inTexCoord + direction * sampleScale * offset;
         outColor += texture(inputTexture, uv) * kMediumQualityWeights[i];
      }
   }
   else if (kRenderQuality == kRenderQuality_High)
   {
      for (int i = 0; i < 7; ++i)
      {
         int offset = i - 3;
         vec2 uv = inTexCoord + direction * sampleScale * offset;
         outColor += texture(inputTexture, uv) * kHighQualityWeights[i];
      }
   }

   if (!kHorizontal)
   {
      vec4 previousColor = texture(previousTexture, inTexCoord);
      outColor = mix(outColor, previousColor, colorMix);
   }
}
