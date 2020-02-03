#pragma once

#include "Core/Assert.h"

#include <vulkan/vulkan.hpp>

#include <optional>
#include <set>
#include <vector>

struct GLFWwindow;

struct QueueFamilyIndices
{
   std::optional<uint32_t> graphicsFamily;
   std::optional<uint32_t> presentFamily;

   bool isComplete() const
   {
      return graphicsFamily && presentFamily;
   }

   std::set<uint32_t> getUniqueIndices() const
   {
      ASSERT(isComplete());

      return { *graphicsFamily, *presentFamily };
   }
};

class ForgeApplication
{
public:
   ForgeApplication();
   ~ForgeApplication();

   void run();

private:
   void initializeGlfw();
   void terminateGlfw();

   void initializeVulkan();
   void terminateVulkan();

   void initializeRenderPass();
   void terminateRenderPass();

   void initializeGraphicsPipeline();
   void terminateGraphicsPipeline();

   void initializeFramebuffers();
   void terminateFramebuffers();

   void initializeCommandBuffers();
   void terminateCommandBuffers();

   GLFWwindow* window = nullptr;

   vk::Instance instance;
   vk::SurfaceKHR surface;
   QueueFamilyIndices queueFamilyIndices;
   vk::Device device;
   vk::Queue graphicsQueue;
   vk::Queue presentQueue;

   vk::SwapchainKHR swapchain;
   std::vector<vk::Image> swapchainImages;
   vk::Format swapchainImageFormat;
   vk::Extent2D swapchainExtent;
   std::vector<vk::ImageView> swapchainImageViews;

   vk::RenderPass renderPass;
   vk::PipelineLayout pipelineLayout;
   vk::Pipeline graphicsPipeline;

   std::vector<vk::Framebuffer> swapchainFramebuffers;

   vk::CommandPool commandPool;
   std::vector<vk::CommandBuffer> commandBuffers;

#if FORGE_DEBUG
   VkDebugUtilsMessengerEXT debugMessenger = nullptr;
#endif // FORGE_DEBUG
};
