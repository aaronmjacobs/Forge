#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "Masked.glsl"

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

layout(location = 0) in vec2 inTexCoord;

void main()
{
   float alpha = texture(albedoTexture, inTexCoord).a;
   if (!passesMaskThreshold(alpha))
   {
      discard;
   }
}
