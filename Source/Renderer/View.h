#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Renderer/ViewInfo.h"

#include <glm/glm.hpp>

#include <vector>

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

class ViewDescriptorSet : public TypedDescriptorSet<ViewDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class View : public GraphicsResource
{
public:
   View(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool);

   void update(const ViewInfo& viewInfo);

   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const
   {
      return uniformBuffer.getDescriptorBufferInfo(frameIndex);
   }

   const ViewDescriptorSet& getDescriptorSet() const
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
   ViewDescriptorSet descriptorSet;

   ViewInfo info;
   ViewMatrices matrices;
};
