#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(push_constant) uniform Mesh
{
    mat4 localToWorld;
} mesh;

layout(location = 0) in vec3 inPosition;

void main()
{
   gl_Position = view.worldToClip * mesh.localToWorld * vec4(inPosition, 1.0);
}
