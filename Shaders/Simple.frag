#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const bool kUseTexture = false;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   vec3 color = inColor;
   if (kUseTexture)
   {
      color *= texture(textureSampler, inTexCoord).rgb;
   }

   outColor = vec4(color, 1.0);
}
