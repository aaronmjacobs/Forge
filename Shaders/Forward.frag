#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable

#include "Lighting.glsl"
#include "View.glsl"

layout(std430, set = 1, binding = 0) uniform Lights
{
	SpotLight spotLights[8];
	PointLight pointLights[8];
	DirectionalLight directionalLights[2];

	int numSpotLights;
	int numPointLights;
	int numDirectionalLights;
};

layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 inTBN;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 tangentSpaceNormal = texture(normalTexture, inTexCoord).rgb * 2.0 - 1.0;

	LightingParams lightingParams;
	lightingParams.diffuseColor = texture(diffuseTexture, inTexCoord).rgb;
	lightingParams.specularColor = vec3(0.5);
	lightingParams.shininess = 30.0;
	lightingParams.surfacePosition = inPosition;
	lightingParams.surfaceNormal = normalize(inTBN * tangentSpaceNormal);
	lightingParams.cameraPosition = view.position.xyz;

	vec3 color = vec3(0.0);

	for (int i = 0; i < numDirectionalLights; ++i)
	{
		color += calcDirectionalLighting(directionalLights[i], lightingParams);
	}

	for (int i = 0; i < numPointLights; ++i)
	{
		color += calcPointLighting(pointLights[i], lightingParams);
	}

	for (int i = 0; i < numSpotLights; ++i)
	{
		color += calcSpotLighting(spotLights[i], lightingParams);
	}

	outColor = vec4(color, 1.0);
}
