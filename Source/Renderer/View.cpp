#include "Renderer/View.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayout.h"

// static
std::array<vk::DescriptorSetLayoutBinding, 1> View::getBindings()
{
   return
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
   };
}

// static
const vk::DescriptorSetLayoutCreateInfo& View::getLayoutCreateInfo()
{
   return DescriptorSetLayout::getCreateInfo<View>();
}

// static
vk::DescriptorSetLayout View::getLayout(const GraphicsContext& context)
{
   return DescriptorSetLayout::get<View>(context);
}

View::View(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, getLayoutCreateInfo())
{
   NAME_CHILD(uniformBuffer, "");
   NAME_CHILD(descriptorSet, "");

   updateDescriptorSets();
}

void View::update(const ViewInfo& viewInfo)
{
   info = viewInfo;
   matrices = ViewMatrices(viewInfo);

   ViewUniformData viewUniformData;

   viewUniformData.worldToView = matrices.worldToView;
   viewUniformData.viewToWorld = glm::inverse(matrices.worldToView);

   viewUniformData.viewToClip = matrices.viewToClip;
   viewUniformData.clipToView = glm::inverse(matrices.viewToClip);

   viewUniformData.worldToClip = matrices.worldToClip;
   viewUniformData.clipToWorld = glm::inverse(matrices.worldToClip);

   viewUniformData.position = glm::vec4(matrices.viewPosition.x, matrices.viewPosition.y, matrices.viewPosition.z, 1.0f);
   viewUniformData.direction = glm::vec4(matrices.viewDirection.x, matrices.viewDirection.y, matrices.viewDirection.z, 0.0f);

   viewUniformData.nearFar = matrices.nearFar;

   uniformBuffer.update(viewUniformData);
}

void View::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet viewDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      device.updateDescriptorSets({ viewDescriptorWrite }, {});
   }
}
