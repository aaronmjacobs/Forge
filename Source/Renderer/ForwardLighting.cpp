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
   bool isDepthFormat(vk::Format format)
   {
      switch (format)
      {
      case vk::Format::eD16Unorm:
      case vk::Format::eD32Sfloat:
      case vk::Format::eD16UnormS8Uint:
      case vk::Format::eD24UnormS8Uint:
      case vk::Format::eD32SfloatS8Uint:
         return true;
      default:
         return false;
      }
   }

   std::unique_ptr<Texture> createShadowMapTextureArray(const GraphicsContext& context, vk::Format format, vk::Extent2D extent, uint32_t layers, bool cubeCompatible)
   {
      bool formatContainsDepth = isDepthFormat(format);

      ImageProperties depthImageProperties;
      depthImageProperties.format = format;
      depthImageProperties.width = extent.width;
      depthImageProperties.height = extent.height;
      depthImageProperties.layers = layers;
      depthImageProperties.cubeCompatible = cubeCompatible;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = vk::SampleCountFlagBits::e1;
      depthTextureProperties.usage = (formatContainsDepth ? vk::ImageUsageFlagBits::eDepthStencilAttachment : vk::ImageUsageFlagBits::eColorAttachment) | vk::ImageUsageFlagBits::eSampled;
      depthTextureProperties.aspects = formatContainsDepth ? (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil) : vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);

      return std::make_unique<Texture>(context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

// static
const vk::DescriptorSetLayoutCreateInfo& ForwardLighting::getLayoutCreateInfo()
{
   static const std::array<vk::DescriptorSetLayoutBinding, 3> kBindings =
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

ForwardLighting::ForwardLighting(const GraphicsContext& graphicsContext, DynamicDescriptorPool& dynamicDescriptorPool, vk::Format depthStencilFormat, vk::Format distanceFormat)
   : GraphicsResource(graphicsContext)
   , uniformBuffer(graphicsContext)
   , descriptorSet(graphicsContext, dynamicDescriptorPool, getLayoutCreateInfo())
{
   NAME_CHILD(uniformBuffer, "");
   NAME_CHILD(descriptorSet, "");

   static const vk::Extent2D kPointShadowResolution = vk::Extent2D(1024, 1024);
   static const vk::Extent2D kSpotShadowResolution = vk::Extent2D(1024, 1024);

   pointShadowMapTextureArray = createShadowMapTextureArray(context, distanceFormat, kPointShadowResolution, kMaxPointShadowMaps * kNumCubeFaces, true);
   NAME_CHILD_POINTER(pointShadowMapTextureArray, "Point Shadow Texture Array");

   pointShadowMapDepthTextureArray = createShadowMapTextureArray(context, depthStencilFormat, kPointShadowResolution, kNumCubeFaces, true);
   NAME_CHILD_POINTER(pointShadowMapDepthTextureArray, "Point Shadow Depth Texture Array");

   spotShadowMapTextureArray = createShadowMapTextureArray(context, depthStencilFormat, kSpotShadowResolution, kMaxSpotShadowMaps, false);
   NAME_CHILD_POINTER(spotShadowMapTextureArray, "Spot Shadow Texture Array");

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

   pointShadowSampleView = pointShadowMapTextureArray->createView(vk::ImageViewType::eCubeArray, 0, kMaxPointShadowMaps * kNumCubeFaces, vk::ImageAspectFlagBits::eColor);
   NAME_CHILD(pointShadowSampleView, "Point Shadow Map Sample View");

   spotShadowSampleView = spotShadowMapTextureArray->createView(vk::ImageViewType::e2DArray, 0, kMaxSpotShadowMaps, vk::ImageAspectFlagBits::eDepth);
   NAME_CHILD(spotShadowSampleView, "Spot Shadow Map Sample View");

   for (uint32_t shadowMapIndex = 0; shadowMapIndex < kMaxPointShadowMaps; ++shadowMapIndex)
   {
      for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
      {
         uint32_t viewIndex = getPointViewIndex(shadowMapIndex, faceIndex);

         pointShadowViews[viewIndex] = pointShadowMapTextureArray->createView(vk::ImageViewType::e2D, viewIndex, 1);
         NAME_CHILD(pointShadowViews[viewIndex], "Point Shadow Map View " + std::to_string(viewIndex));
      }
   }

   for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
   {
      pointShadowDepthViews[faceIndex] = pointShadowMapDepthTextureArray->createView(vk::ImageViewType::e2D, faceIndex, 1);
      NAME_CHILD(pointShadowDepthViews[faceIndex], "Point Shadow Map Depth View " + std::to_string(faceIndex));
   }

   for (uint32_t i = 0; i < kMaxSpotShadowMaps; ++i)
   {
      spotShadowViews[i] = spotShadowMapTextureArray->createView(vk::ImageViewType::e2D, i, 1);
      NAME_CHILD(spotShadowViews[i], "Spot Shadow Map View " + std::to_string(i));
   }

   updateDescriptorSets();
}

ForwardLighting::~ForwardLighting()
{
   context.delayedDestroy(std::move(shadowMapSampler));

   context.delayedDestroy(std::move(pointShadowSampleView));
   context.delayedDestroy(std::move(spotShadowSampleView));

   for (uint32_t shadowMapIndex = 0; shadowMapIndex < kMaxPointShadowMaps; ++shadowMapIndex)
   {
      for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
      {
         uint32_t viewIndex = getPointViewIndex(shadowMapIndex, faceIndex);

         context.delayedDestroy(std::move(pointShadowViews[viewIndex]));
      }
   }

   for (uint32_t faceIndex = 0; faceIndex < kNumCubeFaces; ++faceIndex)
   {
      context.delayedDestroy(std::move(pointShadowDepthViews[faceIndex]));
   }

   for (uint32_t i = 0; i < kMaxSpotShadowMaps; ++i)
   {
      context.delayedDestroy(std::move(spotShadowViews[i]));
   }
}

void ForwardLighting::transitionShadowMapLayout(vk::CommandBuffer commandBuffer, bool forReading)
{
   vk::ImageLayout pointLayout = forReading ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eColorAttachmentOptimal;
   vk::ImageLayout spotLayout = forReading ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal;

   TextureMemoryBarrierFlags pointSrcMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
   TextureMemoryBarrierFlags pointDstMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

   TextureMemoryBarrierFlags spotSrcMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader);
   TextureMemoryBarrierFlags spotDstMemoryBarrierFlags = forReading ? TextureMemoryBarrierFlags(vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader) : TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

   pointShadowMapTextureArray->transitionLayout(commandBuffer, pointLayout, pointSrcMemoryBarrierFlags, pointDstMemoryBarrierFlags);
   spotShadowMapTextureArray->transitionLayout(commandBuffer, spotLayout, spotSrcMemoryBarrierFlags, spotDstMemoryBarrierFlags);
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
      uniformData.color = glm::vec4(directionalLightRenderInfo.color, 0.0f);
      uniformData.direction = glm::vec4(directionalLightRenderInfo.direction, 0.0f);

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

TextureInfo ForwardLighting::getPointShadowDepthInfo(uint32_t faceIndex) const
{
   ASSERT(faceIndex < kNumCubeFaces);

   TextureInfo info = pointShadowMapDepthTextureArray->getInfo();
   info.view = pointShadowDepthViews[faceIndex];

   return info;
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

      vk::DescriptorImageInfo pointImageInfo = vk::DescriptorImageInfo()
         .setImageLayout(pointShadowMapTextureArray->getLayout())
         .setImageView(pointShadowSampleView)
         .setSampler(shadowMapSampler);

      vk::DescriptorImageInfo spotImageInfo = vk::DescriptorImageInfo()
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

      device.updateDescriptorSets({ bufferDescriptorWrite, pointImageDescriptorWrite, spotImageDescriptorWrite }, {});
   }
}
