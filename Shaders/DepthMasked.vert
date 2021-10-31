#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(push_constant) uniform Mesh
{
    mat4 localToWorld;
} mesh;

layout(location = 0) in vec3 inPosition;
layout(location = 5) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main()
{
   vec4 worldPosition = mesh.localToWorld * vec4(inPosition, 1.0);
   gl_Position = view.worldToClip * worldPosition;

   outTexCoord = inTexCoord;
}
