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
   std::unique_ptr<Texture> createShadowMapTextureArray(const GraphicsContext& context, vk::Format format, vk::Extent2D extent, uint32_t layers, bool cubeCompatible)
   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = format;
      depthImageProperties.width = extent.width;
      depthImageProperties.height = extent.height;
      depthImageProperties.layers = layers;
      depthImageProperties.cubeCompatible = cubeCompatible;

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
   static const std::array<vk::DescriptorSetLayoutBinding, 4> kBindings =
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
      vk::DescriptorSetLayoutBinding()
         .setBinding(2)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setStageFlags(vk::ShaderStageFlagBits::eFragment),
      vk::DescriptorSetLayoutBinding()
         .setBinding(3)
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

// static
uint32_t ForwardLighting::getPointViewIndex(uint32_t shadowMapIndex, uint32_t faceIndex)
{
   return shadowMapIndex * kNumCubeFaces + faceIndex;
}

ForwardLighting::ForwardLighting(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Format depthFormat)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, getLayoutCreateInfo())
{
   NAME_CHILD(uniformBuffer, "");
   NAME_CHILD(descriptorSet, "");

   pointShadowMapTextureArray = createShadowMapTextureArray(context, depthFormat, vk::Extent2D(1024, 1024), kMaxPointShadowMaps * kNumCubeFaces, true);
   NAME_CHILD_POINTER(pointShadowMapTextureArray, "Point Shadow Texture Array");

   spotShadowMapTextureArray = createShadowMapTextureArray(context, depthFormat, vk::Extent2D(1024, 1024), kMaxSpotShadowMaps, false);
   NAME_CHILD_POINTER(spotShadowMapTextureArray, "Spot Shadow Texture Array");

   directionalShadowMapTextureArray = createShadowMapTextureArray(context, depthFormat, vk::Extent2D(4096, 4096), kMaxDirectionalShadowMaps, false);
   NAME_CHILD_POINTER(directionalShadowMapTextureArray, "Directional Shadow Texture Array");

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
   NAME_CHILD(shadowMapSampler, "Shadow Map Sampler");

   pointShadowSampleView = pointShadowMapTextureArray->createView(vk::ImageViewType::eCubeArray, 0, kMaxPointShadowMaps * kNumCubeFaces, vk::ImageAspectFlagBits::eDepth);
   NAME_CHILD(pointShadowSampleView, "Point Shadow Map Sample View");

   spotShadowSampleView = spotShadowMapTextureArray->createView(vk::ImageViewType::e2DArray, 0, kMaxSpotShadowMaps, vk::ImageAspectFlagBits::eDepth);
   NAME_CHILD(spotShadowSampleView, "Spot Shadow Map Sample View");

   directionalShadowSampleView = directionalShadowMapTextureArray->createView(vk::ImageViewType::e2DArray, 0, kMaxDirectionalShadowMaps, vk::ImageAspectFlagBits::eDepth);
   NAME_CHILD(directionalShadowSampleView, "Directional Shadow Map Sample View");

   for (uint32_t shadowMapIndex = 0; shadowMapIndex < kMaxPointShadowMaps; ++shadowMapIndex)
   {
      for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
      {
         uint32_t viewIndex = getPointViewIndex(shadowMapIndex, faceIndex);

         pointShadowViews[viewIndex] = pointShadowMapTextureArray->createView(vk::ImageViewType::e2D, viewIndex, 1);
         NAME_CHILD(pointShadowViews[viewIndex], "Point Shadow Map View " + std::to_string(viewIndex));
      }
   }

   for (uint32_t i = 0; i < kMaxSpotShadowMaps; ++i)
   {
      spotShadowViews[i] = spotShadowMapTextureArray->createView(vk::ImageViewType::e2D, i, 1);
      NAME_CHILD(spotShadowViews[i], "Spot Shadow Map View " + std::to_string(i));
   }

   for (uint32_t i = 0; i < kMaxDirectionalShadowMaps; ++i)
   {
      directionalShadowViews[i] = directionalShadowMapTextureArray->createView(vk::ImageViewType::e2D, i, 1);
      NAME_CHILD(directionalShadowViews[i], "Directional Shadow Map View " + std::to_string(i));
   }

   updateDescriptorSets();
}

ForwardLighting::~ForwardLighting()
{
   context.delayedDestroy(std::move(shadowMapSampler));

   context.delayedDestroy(std::move(pointShadowSampleView));
   context.delayedDestroy(std::move(spotShadowSampleView));
   context.delayedDestroy(std::move(directionalShadowSampleView));

   for (vk::ImageView& pointShadowView : pointShadowViews)
   {
      context.delayedDestroy(std::move(pointShadowView));
   }
   for (vk::ImageView& spotShadowView : spotShadowViews)
   {
      context.delayedDestroy(std::move(spotShadowView));
   }
   for (vk::ImageView& directionalShadowView : directionalShadowViews)
   {
      context.delayedDestroy(std::move(directionalShadowView));
   }
}

void ForwardLighting::transitionShadowMapLayout(vk::CommandBuffer commandBuffer, bool forReading)
{
   vk::ImageLayout layout = forReading ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;

   TextureMemoryBarrierFlags srcMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
   TextureMemoryBarrierFlags dstMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

   pointShadowMapTextureArray->transitionLayout(commandBuffer, layout, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   spotShadowMapTextureArray->transitionLayout(commandBuffer, layout, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
   directionalShadowMapTextureArray->transitionLayout(commandBuffer, layout, srcMemoryBarrierFlags, dstMemoryBarrierFlags);
}

void ForwardLighting::update(const SceneRenderInfo& sceneRenderInfo)
{
   ForwardLightingUniformData lightUniformData;

   for (const PointLightRenderInfo& pointLightRenderInfo : sceneRenderInfo.pointLights)
   {
      ForwardPointLightUniformData& uniformData = lightUniformData.pointLights[lightUniformData.numPointLights];
      uniformData.colorRadius = glm::vec4(pointLightRenderInfo.color.x, pointLightRenderInfo.color.y, pointLightRenderInfo.color.z, pointLightRenderInfo.radius);
      uniformData.position = glm::vec4(pointLightRenderInfo.position.x, pointLightRenderInfo.position.y, pointLightRenderInfo.position.z, 0.0f);
      uniformData.nearFar = glm::vec2(pointLightRenderInfo.shadowNearPlane, pointLightRenderInfo.radius);
      uniformData.shadowMapIndex = pointLightRenderInfo.shadowMapIndex.value_or(-1);

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
      ViewMatrices viewMatrices;
      if (directionalLightRenderInfo.shadowViewInfo.has_value())
      {
         viewMatrices = ViewMatrices(directionalLightRenderInfo.shadowViewInfo.value());
      }

      uniformData.worldToShadow = viewMatrices.worldToClip;
      uniformData.color = glm::vec4(directionalLightRenderInfo.color, 0.0f);
      uniformData.direction = glm::vec4(directionalLightRenderInfo.direction, 0.0f);
      uniformData.nearFar = glm::vec2(-directionalLightRenderInfo.shadowOrthoDepth, directionalLightRenderInfo.shadowOrthoDepth);
      uniformData.shadowMapIndex = directionalLightRenderInfo.shadowMapIndex.value_or(-1);

      if (++lightUniformData.numDirectionalLights >= lightUniformData.directionalLights.size())
      {
         break;
      }
   }

   uniformBuffer.update(lightUniformData);
}

TextureInfo ForwardLighting::getPointShadowInfo(uint32_t shadowMapIndex, uint32_t faceIndex) const
{
   ASSERT(shadowMapIndex < kMaxPointShadowMaps);
   ASSERT(faceIndex < kNumCubeFaces);

   uint32_t viewIndex = getPointViewIndex(shadowMapIndex, faceIndex);

   TextureInfo info = pointShadowMapTextureArray->getInfo();
   info.view = pointShadowViews[viewIndex];

   return info;
}

TextureInfo ForwardLighting::getSpotShadowInfo(uint32_t index) const
{
   ASSERT(index < kMaxSpotShadowMaps);

   TextureInfo info = spotShadowMapTextureArray->getInfo();
   info.view = spotShadowViews[index];

   return info;
}

TextureInfo ForwardLighting::getDirectionalShadowInfo(uint32_t index) const
{
   ASSERT(index < kMaxDirectionalShadowMaps);

   TextureInfo info = directionalShadowMapTextureArray->getInfo();
   info.view = directionalShadowViews[index];

   return info;
}

void ForwardLighting::updateDescriptorSets()
{
   for (uint32_t frameIndex = 0; frameIndex < GraphicsContext::kMaxFramesInFlight; ++frameIndex)
   {
      vk::DescriptorBufferInfo viewBufferInfo = uniformBuffer.getDescriptorBufferInfo(frameIndex);

      vk::DescriptorImageInfo pointImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(pointShadowMapTextureArray->getLayout())
         .setImageView(pointShadowSampleView)
         .setSampler(shadowMapSampler);

      vk::DescriptorImageInfo spotImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(spotShadowMapTextureArray->getLayout())
         .setImageView(spotShadowSampleView)
         .setSampler(shadowMapSampler);

      vk::DescriptorImageInfo directionalImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(directionalShadowMapTextureArray->getLayout())
         .setImageView(directionalShadowSampleView)
         .setSampler(shadowMapSampler);

      vk::WriteDescriptorSet bufferDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(0)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eUniformBuffer)
         .setDescriptorCount(1)
         .setPBufferInfo(&viewBufferInfo);

      vk::WriteDescriptorSet pointImageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(1)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&pointImageInfo);

      vk::WriteDescriptorSet spotImageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(2)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&spotImageInfo);

      vk::WriteDescriptorSet directionalImageDescriptorWrite = vk::WriteDescriptorSet()
         .setDstSet(descriptorSet.getSet(frameIndex))
         .setDstBinding(3)
         .setDstArrayElement(0)
         .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
         .setDescriptorCount(1)
         .setPImageInfo(&directionalImageInfo);

      device.updateDescriptorSets({ bufferDescriptorWrite, pointImageDescriptorWrite, spotImageDescriptorWrite, directionalImageDescriptorWrite }, {});
   }
}
