#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(location = 0) in vec3 inPosition;

void main()
{
	vec3 toViewPosition = view.position.xyz - inPosition;
	gl_FragDepth = length(toViewPosition);
}
