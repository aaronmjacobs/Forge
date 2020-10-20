#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   outColor = vec4(inColor * texture(textureSampler, inTexCoord * 2.0).rgb, 1.0);
}
