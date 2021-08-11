#pragma once

#include "Graphics/DescriptorSet.h"
#include "Graphics/GraphicsResource.h"
#include "Graphics/UniformBuffer.h"

#include "Math/Transform.h"

#include "Renderer/ViewInfo.h"

#include <glm/glm.hpp>

struct ViewUniformData
{
   alignas(16) glm::mat4 worldToClip;
   alignas(16) glm::vec4 position;
   alignas(16) glm::vec4 direction;
   alignas(16) glm::vec2 nearFar;
};

class View : public GraphicsResource
{
public:
   static const vk::DescriptorSetLayoutCreateInfo& getLayoutCreateInfo();
   static vk::DescriptorSetLayout getLayout(const GraphicsContext& context);

   View(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool);

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

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

private:
   void updateDescriptorSets();

   UniformBuffer<ViewUniformData> uniformBuffer;
   DescriptorSet descriptorSet;

   ViewInfo info;
   ViewMatrices matrices;
};
