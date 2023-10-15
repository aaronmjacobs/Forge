#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "View.glsl"

layout(set = 1, binding = 0) uniform sampler2D depthBuffer;
layout(set = 1, binding = 1) uniform sampler2D normalBuffer;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec3 outReflectedUV;

void main()
{
   vec3 position = getViewPosition(depthBuffer, inTexCoord);
   vec3 normal = getViewNormal(normalBuffer, inTexCoord);
   vec3 pivot = normalize(reflect(normalize(position), normal));

   float maxDistance = 15.0;
   float resolution = 0.3; // 0 = no reflection, 1 = pixel by pixel
   int steps = 10;
   float thickness = 0.5;

   vec3 startView = position;
   vec3 endView = position + (pivot * maxDistance);

   vec2 texSize  = textureSize(depthBuffer, 0).xy;
   vec2 startFrag = viewToFragment(startView, texSize);
   vec2 endFrag = viewToFragment(endView, texSize);

   outReflectedUV = vec3(inTexCoord, 0.5f);
}
