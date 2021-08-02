#include "Renderer/ForwardLighting.h"

#include "Core/Assert.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/Texture.h"

#include "Renderer/SceneRenderInfo.h"

#include <array>
#include <utility>

namespace
{
   std::unique_ptr<Texture> createShadowMapTextureArray(const GraphicsContext& context, vk::Format format, vk::Extent2D extent, uint32_t layers)
   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = format;
      depthImageProperties.width = extent.width;
      depthImageProperties.height = extent.height;
      depthImageProperties.layers = layers;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = vk::SampleCountFlagBits::e1;
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
      depthTextureProperties.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

      return std::make_unique<Texture>(context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

// static
const vk::DescriptorSetLayoutCreateInfo& ForwardLighting::getLayoutCreateInfo()
{
   static const std::array<vk::DescriptorSetLayoutBinding, 2> kBindings =
   {
      vk::DescriptorSetLayoutBinding()
         .setBinding(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(1)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
   };

   static const vk::DescriptorSetLayoutCreateInfo kCreateInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), kBindings);

   return kCreateInfo;
}

// static
vk::DescriptorSetLayout ForwardLighting::getLayout(const GraphicsContext& context)
{
   return context.getLayoutCache().getLayout(getLayoutCreateInfo());
}

ForwardLighting::ForwardLighting(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, vk::Format depthFormat)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, descriptorPool, getLayoutCreateInfo())
{
   NAME_OBJECT(uniformBuffer, "Forward Lighting");
   NAME_OBJECT(descriptorSet, "Forward Lighting");

   spotShadowMapTextureArray = createShadowMapTextureArray(context, depthFormat, vk::Extent2D(1024, 1024), kMaxSpotShadowMaps);
   NAME_POINTER(spotShadowMapTextureArray, "Spot Shadow Texture Array");

   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
      .setAddressModeW(vk::SamplerAddressMode::eClampToBorder)
      .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
      .setAnisotropyEnable(false)
      .setMaxAnisotropy(1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(true)
      .setCompareOp(vk::CompareOp::eLessOrEqual)
      .setMipmapMode(vk::SamplerMipmapMode::eNearest)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   shadowMapSampler = context.getDevice().createSampler(samplerCreateInfo);
   NAME_OBJECT(descriptorSet, "Shadow Map Sampler");

   spotShadowSampleView = spotShadowMapTextureArray->createView(vk::ImageViewType::e2DArray, 0, kMaxSpotShadowMaps, vk::ImageAspectFlagBits::eDepth);

   for (uint32_t i = 0; i < kMaxSpotShadowMaps; ++i)
   {
      spotShadowViews[i] = spotShadowMapTextureArray->createView(vk::ImageViewType::e2D, i, 1);
      NAME_OBJECT(spotShadowViews[i], "Spot Shadow Map Sampler " + std::to_string(i));
   }

   updateDescriptorSets();
}

ForwardLighting::~ForwardLighting()
{
   context.delayedDestroy(std::move(shadowMapSampler));

   context.delayedDestroy(std::move(spotShadowSampleView));

   for (uint32_t i = 0; i < kMaxSpotShadowMaps; ++i)
   {
      context.delayedDestroy(std::move(spotShadowViews[i]));
   }
}

void ForwardLighting::transitionShadowMapLayout(vk::CommandBuffer commandBuffer, bool forReading)
{
   vk::ImageLayout layout = forReading ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;

   TextureMemoryBarrierFlags srcMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
   TextureMemoryBarrierFlags dstMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);
   spotShadowMapTextureArray->transitionLayout(commandBuffer, layout, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
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
      ViewMatrices viewMatrices;
      if (spotLightRenderInfo.shadowViewInfo.has_value())
      {
         viewMatrices = ViewMatrices(spotLightRenderInfo.shadowViewInfo.value());
      }

      uniformData.worldToShadow = viewMatrices.worldToClip;
      uniformData.colorRadius = glm::vec4(spotLightRenderInfo.color.x, spotLightRenderInfo.color.y, spotLightRenderInfo.color.z, spotLightRenderInfo.radius);
      uniformData.positionBeamAngle = glm::vec4(spotLightRenderInfo.position.x, spotLightRenderInfo.position.y, spotLightRenderInfo.position.z, spotLightRenderInfo.beamAngle);
      uniformData.directionCutoffAngle = glm::vec4(spotLightRenderInfo.direction.x, spotLightRenderInfo.direction.y, spotLightRenderInfo.direction.z, spotLightRenderInfo.cutoffAngle);
      uniformData.shadowMapIndex = spotLightRenderInfo.shadowMapIndex.value_or(-1);

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

TextureInfo ForwardLighting::getSpotShadowInfo(uint32_t index) const
{
   ASSERT(index < kMaxSpotShadowMaps);

   TextureInfo info = spotShadowMapTextureArray->getInfo();
   info.view = spotShadowViews[index];

   return info;
}

void ForwardLighting::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

      vk::DescriptorImageInfo imageInfo = vk::DescriptorImageInfo()
         .setImageLayout(spotShadowMapTextureArray->getLayout())
         .setImageView(spotShadowSampleView)
         .setSampler(shadowMapSampler);

      vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      vk::WriteDescriptorSet imageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&imageInfo);

      device.updateDescriptorSets({ bufferDescriptorWrite, imageDescriptorWrite }, {});
   }
}
