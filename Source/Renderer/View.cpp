#include "Renderer/View.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/Swapchain.h"

#include "Math/MathUtils.h"

// static
const vk::DescriptorSetLayoutCreateInfo& View::getLayoutCreateInfo()
{
   static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

   static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

   return kCreateInfo;
}

// static
vk::DescriptorSetLayout View::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

View::View(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   updateDescriptorSets();
}

void View::update(const ViewInfo& viewInfo)
{
   viewMatrices = ViewMatrices(viewInfo);

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewMatrices.viewToClip * viewMatrices.worldToView;
   viewUniformData.position = glm::vec4(viewMatrices.viewPosition.x, viewMatrices.viewPosition.y, viewMatrices.viewPosition.z, 1.0f);
   viewUniformData.direction = glm::vec4(viewMatrices.viewDirection.x, viewMatrices.viewDirection.y, viewMatrices.viewDirection.z, 0.0f);

   uniformBuffer.update(viewUniformData);
}

#if FORGE_DEBUG
void View::setName(std::string_view newName)
{
   GraphicsResource::setName(newName);

   NAME_OBJECT(uniformBuffer, name);
   NAME_OBJECT(descriptorSet, name);
}
#endif // FORGE_DEBUG

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
