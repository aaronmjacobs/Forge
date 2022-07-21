#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

#include "View.glsl"

const uint kNumSamples = 32;
const uint kNoiseSize = 4;
const uint kNumNoiseValues = (kNoiseSize * kNoiseSize) / 2; // Half size since only two elements per value

layout(set = 1, binding = 0) uniform sampler2D depthBuffer;
layout(set = 1, binding = 1) uniform sampler2D normalBuffer;

layout(std430, set = 1, binding = 2) uniform SSAOData
{
   vec4 samples[kNumSamples];
   vec4 noise[kNumNoiseValues];
};

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out float outSSAO;

vec3 getViewPosition(vec2 uv)
{
   float depth = texture(depthBuffer, uv).r;
   float z = depth * 2.0 - 1.0;

   vec4 clipPosition = vec4(uv * 2.0 - 1.0, z, 1.0);
   vec4 viewPosition = view.clipToView * clipPosition;
   viewPosition /= viewPosition.w;

   return viewPosition.xyz;
}

vec3 randomVec()
{
   uvec2 noiseCoord = uvec2(gl_FragCoord.xy) % kNoiseSize;
   uint arrayIndex = (noiseCoord.x + noiseCoord.y * kNoiseSize) / 2;
   uint elementIndex = (noiseCoord.x + noiseCoord.y * kNoiseSize) % 2;

   vec4 noiseElements = noise[arrayIndex];
   vec2 noiseValues = elementIndex == 0 ? noiseElements.xy : noiseElements.zw;

   return vec3(noiseValues, 0.0);
}

void main()
{
   vec3 position = getViewPosition(inTexCoord);
   vec3 normal = normalize(view.worldToView * vec4(texture(normalBuffer, inTexCoord).xyz, 0.0)).xyz;
   vec3 randVec = randomVec();

   vec3 tangent = normalize(randVec - normal * dot(randVec, normal));
   vec3 bitangent = cross(normal, tangent);
   mat3 tbn = mat3(tangent, bitangent, normal);

   outSSAO = 0.0;
   for (uint i = 0; i < kNumSamples; ++i)
   {
      float radius = 1.0;
      vec3 samplePosition = position + (tbn * samples[i].xyz) * radius;

      vec4 offset = view.viewToClip * vec4(samplePosition, 1.0);
      offset.xyz = (offset.xyz / offset.w) * 0.5 + 0.5;
      float sampleDepth = getViewPosition(offset.xy).z;

      float rangeCheck = smoothstep(0.0, 1.0, radius / abs(position.z - sampleDepth));
      const float kBias = 0.025;
      outSSAO += (sampleDepth >= samplePosition.z + kBias ? 1.0 : 0.0) * rangeCheck;
   }

   const float kStrength = 1.0;
   outSSAO = pow(1.0 - (outSSAO / kNumSamples), kStrength);
}
