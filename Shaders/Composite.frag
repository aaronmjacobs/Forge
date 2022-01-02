#version 450
#extension GL_ARB_separate_shader_objects : enable

const int kModePassthrough = 0;
const int kModeLinearToSrgb = 1;
const int kModeSrgbToLinear = 2;

layout(constant_id = 0) const int kMode = kModePassthrough;

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

vec3 linearToSrgb(vec3 linearRGB)
{
   vec3 a = 12.92 * linearRGB;
   vec3 b = 1.055 * pow(linearRGB, vec3(1.0 / 2.4)) - 0.055;
   vec3 c = step(vec3(0.0031308), linearRGB);
   return mix(a, b, c);
}

vec3 srgbToLinear(vec3 sRGB)
{
   vec3 a = sRGB / 12.92;
   vec3 b = pow((sRGB + 0.055) / 1.055, vec3(2.4));
   vec3 c = step(vec3(0.04045), sRGB);
   return mix(a, b, c);
}

void main()
{
   vec4 sampledColor = texture(sourceTexture, inTexCoord);

   if (kMode == kModeLinearToSrgb)
   {
      outColor = vec4(linearToSrgb(sampledColor.rgb), sampledColor.a);
   }
   else if (kMode == kModeSrgbToLinear)
   {
      outColor = vec4(srgbToLinear(sampledColor.rgb), sampledColor.a);
   }
   else
   {
      outColor = sampledColor;
   }
}
