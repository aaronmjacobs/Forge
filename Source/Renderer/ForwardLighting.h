#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/TextureInfo.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/ViewInfo.h"

#include <glm/glm.hpp>

#include <memory>

class DynamicDescriptorPool;
class Texture;
struct SceneRenderInfo;

struct ForwardSpotLightUniformData
{
   alignas(16) glm::mat4 worldToShadow;
   alignas(16) glm::vec4 colorRadius;
   alignas(16) glm::vec4 positionBeamAngle;
   alignas(16) glm::vec4 directionCutoffAngle;
   alignas(4) int shadowMapIndex = -1;
};

struct ForwardPointLightUniformData
{
   alignas(16) glm::vec4 colorRadius;
   alignas(16) glm::vec4 position;
   alignas(16) glm::vec2 nearFar;
   alignas(4) int shadowMapIndex = -1;
};

struct ForwardDirectionalLightUniformData
{
   alignas(16) glm::mat4 worldToShadow;
   alignas(16) glm::vec4 color;
   alignas(16) glm::vec4 direction;
   alignas(16) glm::vec2 nearFar;
   alignas(4) int shadowMapIndex = -1;
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

class ForwardLightingDescriptorSet : public TypedDescriptorSet<ForwardLightingDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class ForwardLighting : public GraphicsResource
{
public:
   static const uint32_t kMaxPointShadowMaps = 2;
   static const uint32_t kMaxSpotShadowMaps = 4;
   static const uint32_t kMaxDirectionalShadowMaps = 1;

   static uint32_t getPointViewIndex(uint32_t shadowMapIndex, uint32_t faceIndex);

   ForwardLighting(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Format depthFormat);
   ~ForwardLighting();

   void transitionShadowMapLayout(vk::CommandBuffer commandBuffer, bool forReading);
   void update(const SceneRenderInfo& sceneRenderInfo);

   const ForwardLightingDescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

   Texture& getPointShadowTextureArray()
   {
      return *pointShadowMapTextureArray;
   }

   Texture& getSpotShadowTextureArray()
   {
      return *spotShadowMapTextureArray;
   }

   Texture& getDirectionalShadowTextureArray()
   {
      return *directionalShadowMapTextureArray;
   }

   vk::ImageView getPointShadowView(uint32_t shadowMapIndex, uint32_t faceIndex) const;
   vk::ImageView getSpotShadowView(uint32_t index) const;
   vk::ImageView getDirectionalShadowView(uint32_t index) const;

protected:
   void updateDescriptorSets();

   UniformBuffer<ForwardLightingUniformData> uniformBuffer;
   ForwardLightingDescriptorSet descriptorSet;

   vk::Sampler shadowMapSampler;
   std::unique_ptr<Texture> pointShadowMapTextureArray;
   std::unique_ptr<Texture> spotShadowMapTextureArray;
   std::unique_ptr<Texture> directionalShadowMapTextureArray;
   vk::ImageView pointShadowSampleView;
   vk::ImageView spotShadowSampleView;
   vk::ImageView directionalShadowSampleView;
   std::array<vk::ImageView, kMaxPointShadowMaps * kNumCubeFaces> pointShadowViews;
   std::array<vk::ImageView, kMaxSpotShadowMaps> spotShadowViews;
   std::array<vk::ImageView, kMaxDirectionalShadowMaps> directionalShadowViews;
};
