#if !defined(VIEW_GLSL)
#define VIEW_GLSL

layout(set = 0, binding = 0) uniform View
{
    mat4 worldToClip;
    vec4 position;
    vec4 direction;
    vec2 nearFar;
} view;

#endif
