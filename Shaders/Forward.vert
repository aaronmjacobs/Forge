#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(constant_id = 0) const bool kWithTextures = false;

layout(push_constant) uniform Mesh
{
    mat4 localToWorld;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBitangent;
layout(location = 4) in vec4 inColor;
layout(location = 5) in vec2 inTexCoord;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec2 outTexCoord;

void main()
{
   vec4 worldPosition = mesh.localToWorld * vec4(inPosition, 1.0);
   gl_Position = view.worldToClip * worldPosition;
   outPosition = worldPosition.xyz;

   outColor = inColor;

   if (kWithTextures)
   {
      outTexCoord = inTexCoord;
   }
   else
   {
      outTexCoord = vec2(0.0);
   }
}
