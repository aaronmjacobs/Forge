#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "Masked.glsl"

layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;

layout(location = 0) in vec2 inTexCoord;

void main()
{
   float alpha = texture(diffuseTexture, inTexCoord).a;
   if (!passesMaskThreshold(alpha))
   {
      discard;
   }
}
