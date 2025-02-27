#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

struct FrameUniformData
{
   alignas(16) glm::vec4 midiA;
   alignas(16) glm::vec4 midiB;

   alignas(4) uint32_t number = 0;
   alignas(4) float time = 0.0f;
};

class FrameDescriptorSet : public TypedDescriptorSet<FrameDescriptorSet>
{
public:
   static std::vector<vk::DescriptorSetLayoutBinding> getBindings();

   using TypedDescriptorSet::TypedDescriptorSet;
};

class Frame : public GraphicsResource
{
public:
   Frame(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool);

   void update(uint32_t frameNumber, float time);

   vk::DescriptorBufferInfo getDescriptorBufferInfo(uint32_t frameIndex) const
   {
      return uniformBuffer.getDescriptorBufferInfo(frameIndex);
   }

   const FrameDescriptorSet& getDescriptorSet() const
   {
      return descriptorSet;
   }

private:
   void updateDescriptorSets();

   UniformBuffer<FrameUniformData> uniformBuffer;
   FrameDescriptorSet descriptorSet;
};
