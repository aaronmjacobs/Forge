#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

vec3 tonemap(vec3 hdrColor)
{
   const float curve = 8.0;

   vec3 clampedHdrColor = max(vec3(0.0), hdrColor);
   vec3 sdrColor = clampedHdrColor - (pow(pow(clampedHdrColor, vec3(curve)) + 1.0, vec3(1.0 / curve)) - 1.0);

   return sdrColor;
}

void main()
{
   vec3 hdrColor = texture(hdrTexture, inTexCoord).rgb;

   outColor = vec4(tonemap(hdrColor), 1.0);
}
