#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/ViewInfo.h"

#include <glm/glm.hpp>

#include <array>

class DynamicDescriptorPool;

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToView;
   alignas(16) glm::mat4 viewToWorld;

   alignas(16) glm::mat4 viewToClip;
   alignas(16) glm::mat4 clipToView;

   alignas(16) glm::mat4 worldToClip;
   alignas(16) glm::mat4 clipToWorld;

   alignas(16) glm::vec4 position;
   alignas(16) glm::vec4 direction;

   alignas(16) glm::vec2 nearFar;
};

class View : public GraphicsResource
{
public:
   static std::array<vk::DescriptorSetLayoutBinding, 1> getBindings();
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   View(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool);

   void update(const ViewInfo& viewInfo);

   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const
   {
      return uniformBuffer.getDescriptorBufferInfo(frameIndex);
   }

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

   const ViewInfo& getInfo() const
   {
      return info;
   }

   const ViewMatrices& getMatrices() const
   {
      return matrices;
   }

private:
   void updateDescriptorSets();

   UniformBuffer<ViewUniformData> uniformBuffer;
   DescriptorSet descriptorSet;

   ViewInfo info;
   ViewMatrices matrices;
};
