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

   const glm::mat4& getWorldToView() const
   {
      return worldToView;
   }

   const glm::mat4& getViewToClip() const
   {
      return viewToClip;
   }

   const glm::mat4& getWorldToClip() const
   {
      return worldToClip;
   }

   const glm::vec3& getViewPosition() const
   {
      return viewPosition;
   }

   const glm::vec3& getViewDirection() const
   {
      return viewDirection;
   }

#if FORGE_DEBUG
   void setName(std::string_view newName) override;
#endif // FORGE_DEBUG

private:
   void updateDescriptorSets();

   UniformBuffer<ViewUniformData> uniformBuffer;
   DescriptorSet descriptorSet;

   glm::mat4 worldToView;
   glm::mat4 viewToClip;
   glm::mat4 worldToClip;

   glm::vec3 viewPosition;
   glm::vec3 viewDirection;
};
