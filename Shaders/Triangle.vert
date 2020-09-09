#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(set = 0, binding = 1) uniform Mesh
{
    mat4 localToWorld;
} mesh;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main()
{
   gl_Position = view.worldToClip * mesh.localToWorld * vec4(inPosition, 0.0, 1.0);
   outColor = inColor;
}
