#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(set = 1, binding = 0) uniform samplerCube skyboxTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   vec2 clipCoords = (inTexCoord * 2.0) - 1.0;
   vec4 nearPlanePos = view.clipToWorld * vec4(clipCoords, 0.0, 1.0);
   vec3 direction = normalize((nearPlanePos.xyz / nearPlanePos.w) - view.position.xyz);

   outColor = vec4(texture(skyboxTexture, direction).rgb, 1.0);
}
