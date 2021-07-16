#include "Renderer/Passes/Forward/ForwardLighting.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"

#include "Renderer/SceneRenderInfo.h"

// static
const vk::DescriptorSetLayoutCreateInfo& ForwardLighting::getLayoutCreateInfo()
{
   static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eFragment);

   static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), 1, &kBinding);

   return kCreateInfo;
}

// static
vk::DescriptorSetLayout ForwardLighting::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

ForwardLighting::ForwardLighting(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   NAME_OBJECT(uniformBuffer, "Forward Lighting");
   NAME_OBJECT(descriptorSet, "Forward Lighting");

   updateDescriptorSets();
}

void ForwardLighting::update(const SceneRenderInfo& sceneRenderInfo)
{
   ForwardLightingUniformData lightUniformData;

   for (const PointLightRenderInfo& pointLightRenderInfo : sceneRenderInfo.pointLights)
   {
      ForwardPointLightUniformData& uniformData = lightUniformData.pointLights[lightUniformData.numPointLights];
      uniformData.colorRadius = glm::vec4(pointLightRenderInfo.color.x, pointLightRenderInfo.color.y, pointLightRenderInfo.color.z, pointLightRenderInfo.radius);
      uniformData.position = glm::vec4(pointLightRenderInfo.position.x, pointLightRenderInfo.position.y, pointLightRenderInfo.position.z, 0.0f);

      if (++lightUniformData.numPointLights >= lightUniformData.pointLights.size())
      {
         break;
      }
   }

   for (const SpotLightRenderInfo& spotLightRenderInfo : sceneRenderInfo.spotLights)
   {
      ForwardSpotLightUniformData& uniformData = lightUniformData.spotLights[lightUniformData.numSpotLights];
      uniformData.colorRadius = glm::vec4(spotLightRenderInfo.color.x, spotLightRenderInfo.color.y, spotLightRenderInfo.color.z, spotLightRenderInfo.radius);
      uniformData.positionBeamAngle = glm::vec4(spotLightRenderInfo.position.x, spotLightRenderInfo.position.y, spotLightRenderInfo.position.z, spotLightRenderInfo.beamAngle);
      uniformData.directionCutoffAngle = glm::vec4(spotLightRenderInfo.direction.x, spotLightRenderInfo.direction.y, spotLightRenderInfo.direction.z, spotLightRenderInfo.cutoffAngle);

      if (++lightUniformData.numSpotLights >= lightUniformData.spotLights.size())
      {
         break;
      }
   }

   for (const DirectionalLightRenderInfo& directionalLightRenderInfo : sceneRenderInfo.directionalLights)
   {
      ForwardDirectionalLightUniformData& uniformData = lightUniformData.directionalLights[lightUniformData.numDirectionalLights];
      uniformData.color = glm::vec4(directionalLightRenderInfo.color, 0.0f);
      uniformData.direction = glm::vec4(directionalLightRenderInfo.direction, 0.0f);

      if (++lightUniformData.numDirectionalLights >= lightUniformData.directionalLights.size())
      {
         break;
      }
   }

   uniformBuffer.update(lightUniformData);
}

void ForwardLighting::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

      vk::WriteDescriptorSet descriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      device.updateDescriptorSets({ descriptorWrite }, {});
   }
}
