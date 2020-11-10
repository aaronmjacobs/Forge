#include "Graphics/Context.h"

// static
std::optional<QueueFamilyIndices> QueueFamilyIndices::get(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
{
   std::optional<uint32_t> graphicsFamilyIndex;
   std::optional<uint32_t> presentFamilyIndex;

   uint32_t index = 0;
   for (const vk::QueueFamilyProperties& queueFamilyProperties : physicalDevice.getQueueFamilyProperties())
   {
      if (!graphicsFamilyIndex && (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics))
      {
         graphicsFamilyIndex = index;
      }

      if (!presentFamilyIndex && physicalDevice.getSurfaceSupportKHR(index, surface))
      {
         presentFamilyIndex = index;
      }

      if (graphicsFamilyIndex && presentFamilyIndex)
      {
         QueueFamilyIndices indices;
         indices.graphicsFamily = *graphicsFamilyIndex;
         indices.presentFamily = *presentFamilyIndex;

         return indices;
      }

      ++index;
   }

   return {};
}
