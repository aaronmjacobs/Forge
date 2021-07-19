#include "Renderer/View.h"

#include "Graphics/DebugUtils.h"
#include "Graphics/DescriptorSetLayoutCache.h"
#include "Graphics/Swapchain.h"

#include "Math/MathUtils.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
   glm::vec3 computeViewDirection(const ViewInfo& viewInfo)
   {
      if (viewInfo.projectionMode == ProjectionMode::Perspective)
      {
         switch (viewInfo.perspectiveInfo.direction)
         {
         case PerspectiveDirection::Forward:
            return MathUtils::kForwardVector;
         case PerspectiveDirection::Backward:
            return -MathUtils::kForwardVector;
         case PerspectiveDirection::Right:
            return MathUtils::kRightVector;
         case PerspectiveDirection::Left:
            return -MathUtils::kRightVector;
         case PerspectiveDirection::Up:
            return MathUtils::kUpVector;
         case PerspectiveDirection::Down:
            return -MathUtils::kUpVector;
         default:
            break;
         }
      }

      return viewInfo.transform.getForwardVector();
   }

   glm::mat4 computeViewToClip(const ViewInfo& viewInfo)
   {
      glm::mat4 viewToClip;

      if (viewInfo.projectionMode == ProjectionMode::Orthographic)
      {
         viewToClip = glm::ortho(-viewInfo.orthographicInfo.width, viewInfo.orthographicInfo.width, -viewInfo.orthographicInfo.height, viewInfo.orthographicInfo.height, -viewInfo.orthographicInfo.depth, viewInfo.orthographicInfo.depth);
      }
      else
      {
         viewToClip = glm::perspective(glm::radians(viewInfo.perspectiveInfo.fieldOfView), viewInfo.perspectiveInfo.aspectRatio, viewInfo.perspectiveInfo.nearPlane, viewInfo.perspectiveInfo.farPlane);
      }

      viewToClip[1][1] *= -1.0f; // In Vulkan, the clip space Y coordinates are inverted compared to OpenGL (which GLM was written for), so we flip the sign here

      return viewToClip;
   }
}

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
   viewPosition = viewInfo.transform.position;
   viewDirection = computeViewDirection(viewInfo);

   glm::vec3 upVector = glm::all(glm::epsilonEqual(viewDirection, MathUtils::kUpVector, MathUtils::kKindaSmallNumber)) ? -MathUtils::kForwardVector : MathUtils::kUpVector;
   worldToView = glm::lookAt(viewPosition, viewPosition + viewDirection, upVector);
   viewToClip = computeViewToClip(viewInfo);
   worldToClip = viewToClip * worldToView;

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;
   viewUniformData.position = glm::vec4(viewPosition.x, viewPosition.y, viewPosition.z, 1.0f);
   viewUniformData.direction = glm::vec4(viewDirection.x, viewDirection.y, viewDirection.z, 0.0f);

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
