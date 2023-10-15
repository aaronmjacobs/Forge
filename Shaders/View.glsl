#if !defined(VIEW_GLSL)
#define VIEW_GLSL

layout(set = 0, binding = 0) uniform View
{
   mat4 worldToView;
   mat4 viewToWorld;

   mat4 viewToClip;
   mat4 clipToView;

   mat4 worldToClip;
   mat4 clipToWorld;

   vec4 position;
   vec4 direction;

   vec2 nearFar;
} view;

vec3 getViewPosition(in sampler2D depthTexture, vec2 uv)
{
   float depth = texture(depthTexture, uv).r;
   float z = depth * 2.0 - 1.0;

   vec4 clipPosition = vec4(uv * 2.0 - 1.0, z, 1.0);
   vec4 viewPosition = view.clipToView * clipPosition;
   viewPosition /= viewPosition.w;

   return viewPosition.xyz;
}

vec3 getViewNormal(in sampler2D normalTexture, vec2 uv)
{
   return normalize(view.worldToView * vec4(texture(normalTexture, uv).xyz, 0.0)).xyz;
}

vec2 viewToFragment(vec3 viewPosition, vec2 textureSize)
{
   vec4 clipPosition = view.viewToClip * vec4(viewPosition, 1.0);
   clipPosition /= clipPosition.w;

   return ((clipPosition.xy * 0.5) + 0.5) * textureSize;
}

#endif
