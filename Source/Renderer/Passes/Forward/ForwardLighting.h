#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include <glm/glm.hpp>

struct SceneRenderInfo;

struct ForwardSpotLightUniformData
{
   alignas(16) glm::vec4 colorRadius;
   alignas(16) glm::vec4 positionBeamAngle;
   alignas(16) glm::vec4 directionCutoffAngle;
};

struct ForwardPointLightUniformData
{
   alignas(16) glm::vec4 colorRadius;
   alignas(16) glm::vec4 position;
};

struct ForwardDirectionalLightUniformData
{
   alignas(16) glm::vec4 color;
   alignas(16) glm::vec4 direction;
};

struct ForwardLightingUniformData
{
   std::array<ForwardSpotLightUniformData, 8> spotLights;
   std::array<ForwardPointLightUniformData, 8> pointLights;
   std::array<ForwardDirectionalLightUniformData, 2> directionalLights;

   alignas(4) int numSpotLights = 0;
   alignas(4) int numPointLights = 0;
   alignas(4) int numDirectionalLights = 0;
};

class ForwardLighting : public GraphicsResource
{
public:
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   ForwardLighting(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool);
   ~ForwardLighting() = default;

   void update(const SceneRenderInfo& sceneRenderInfo);

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

protected:
   void updateDescriptorSets();

   UniformBuffer<ForwardLightingUniformData> uniformBuffer;
   DescriptorSet descriptorSet;
};
