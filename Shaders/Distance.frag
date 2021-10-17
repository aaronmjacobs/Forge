#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(location = 0) in vec3 inPosition;

layout(location = 0) out float outDistance;

void main()
{
	vec3 toViewPosition = view.position.xyz - inPosition;
	outDistance = length(toViewPosition);
}
