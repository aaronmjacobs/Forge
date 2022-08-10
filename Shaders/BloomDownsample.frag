#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
   vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
   float x = texelSize.x;
   float y = texelSize.y;

   vec2 uv = inTexCoord;

   vec4 a = texture(inputTexture, vec2(uv.x - 2*x, uv.y + 2*y));
   vec4 b = texture(inputTexture, vec2(uv.x,       uv.y + 2*y));
   vec4 c = texture(inputTexture, vec2(uv.x + 2*x, uv.y + 2*y));

   vec4 d = texture(inputTexture, vec2(uv.x - 2*x, uv.y));
   vec4 e = texture(inputTexture, vec2(uv.x,       uv.y));
   vec4 f = texture(inputTexture, vec2(uv.x + 2*x, uv.y));

   vec4 g = texture(inputTexture, vec2(uv.x - 2*x, uv.y - 2*y));
   vec4 h = texture(inputTexture, vec2(uv.x,       uv.y - 2*y));
   vec4 i = texture(inputTexture, vec2(uv.x + 2*x, uv.y - 2*y));

   vec4 j = texture(inputTexture, vec2(uv.x - x, uv.y + y));
   vec4 k = texture(inputTexture, vec2(uv.x + x, uv.y + y));
   vec4 l = texture(inputTexture, vec2(uv.x - x, uv.y - y));
   vec4 m = texture(inputTexture, vec2(uv.x + x, uv.y - y));

   outColor = e*0.125;
   outColor += (a+c+g+i)*0.03125;
   outColor += (b+d+f+h)*0.0625;
   outColor += (j+k+l+m)*0.125;
}
