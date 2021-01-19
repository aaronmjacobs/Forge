#include "Renderer/View.h"

#include "Graphics/DescriptorSetLayoutCache.h"

#include "Math/MathUtils.h"

#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
   struct ViewInfo
   {
      Transform transform;
      float fieldOfView = 70.0f;
      float nearPlane = 0.1f;
      float farPlane = 100.0f;
   };

   ViewInfo getViewInfo(const Scene& scene)
   {
      ViewInfo viewInfo;

      if (Entity cameraEntity = scene.getActiveCamera())
      {
         if (const TransformComponent* transformComponent = cameraEntity.tryGetComponent<TransformComponent>())
         {
            viewInfo.transform = transformComponent->getAbsoluteTransform();
         }

         if (const CameraComponent* cameraComponent = cameraEntity.tryGetComponent<CameraComponent>())
         {
            viewInfo.fieldOfView = cameraComponent->fieldOfView;
            viewInfo.nearPlane = cameraComponent->nearPlane;
            viewInfo.farPlane = cameraComponent->farPlane;
         }
      }

      return viewInfo;
   }
}

// static
const vk::DescriptorSetLayoutCreateInfo& View::getLayoutCreateInfo()
{
   static const vk::DescriptorSetLayoutBinding kBinding = vk::DescriptorSetLayoutBinding()
      .setBinding(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

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
}

void View::update(const Scene& scene)
{
   ViewInfo viewInfo = getViewInfo(scene);

   glm::mat4 worldToView = glm::lookAt(viewInfo.transform.position, viewInfo.transform.position + viewInfo.transform.getForwardVector(), MathUtils::kUpVector);

   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
   glm::mat4 viewToClip = glm::perspective(glm::radians(viewInfo.fieldOfView), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, viewInfo.nearPlane, viewInfo.farPlane);
   viewToClip[1][1] *= -1.0f; // In Vulkan, the clip space Y coordinates are inverted compared to OpenGL (which GLM was written for), so we flip the sign here

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

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
