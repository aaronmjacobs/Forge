#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D inputTexture;
layout(set = 0, binding = 1) uniform sampler2D previousTexture;
layout(set = 0, binding = 2) uniform BloomParams
{
   float filterRadius;
   float colorMix;
};

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   // The filter kernel is applied with a radius, specified in texture
   // coordinates, so that the radius will vary across mip resolutions.
   vec2 inputTextureSize = textureSize(inputTexture, 0);
   vec2 texelSize = 1.0 / inputTextureSize;
   float x = filterRadius / inputTextureSize.x;
   float y = filterRadius / inputTextureSize.y;

   vec2 uv = inTexCoord;

   // Take 9 samples around current texel:
   // a - b - c
   // d - e - f
   // g - h - i
   // === ('e' is the current texel) ===
   vec4 a = texture(inputTexture, vec2(uv.x - x, uv.y + y));
   vec4 b = texture(inputTexture, vec2(uv.x,     uv.y + y));
   vec4 c = texture(inputTexture, vec2(uv.x + x, uv.y + y));

   vec4 d = texture(inputTexture, vec2(uv.x - x, uv.y));
   vec4 e = texture(inputTexture, vec2(uv.x,     uv.y));
   vec4 f = texture(inputTexture, vec2(uv.x + x, uv.y));

   vec4 g = texture(inputTexture, vec2(uv.x - x, uv.y - y));
   vec4 h = texture(inputTexture, vec2(uv.x,     uv.y - y));
   vec4 i = texture(inputTexture, vec2(uv.x + x, uv.y - y));

   // Apply weighted distribution, by using a 3x3 tent filter:
   //  1   | 1 2 1 |
   // -- * | 2 4 2 |
   // 16   | 1 2 1 |
   outColor = e*4.0;
   outColor += (b+d+f+h)*2.0;
   outColor += (a+c+g+i);
   outColor *= 1.0 / 16.0;

   vec4 previousColor = texture(previousTexture, inTexCoord);
   outColor = mix(outColor, previousColor, colorMix);
}
