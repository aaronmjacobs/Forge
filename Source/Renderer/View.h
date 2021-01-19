#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include <glm/glm.hpp>

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToClip;
};

class View : public GraphicsResource
{
public:
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   View(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool);

   void update();
   void updateDescriptorSets();

   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const
   {
      return uniformBuffer.getDescriptorBufferInfo(frameIndex);
   }

   const DescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

private:
   UniformBuffer<ViewUniformData> uniformBuffer;
   DescriptorSet descriptorSet;
};
