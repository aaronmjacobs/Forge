#include "ForgeApplication.h"

#include "Graphics/Command.h"
#include "Graphics/Memory.h"
#include "Graphics/Swapchain.h"

#include "Platform/Window.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
   const std::size_t kMaxFramesInFlight = 2;

   vk::SampleCountFlagBits getMaxSampleCount(const GraphicsContext& context)
   {
      const vk::PhysicalDeviceLimits& limits = context.getPhysicalDeviceProperties().limits;
      vk::SampleCountFlags flags = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;

      if (flags & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
      if (flags & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
      if (flags & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
      if (flags & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
      if (flags & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
      if (flags & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

      return vk::SampleCountFlagBits::e1;
   }
}

ForgeApplication::ForgeApplication()
{
   initializeGlfw();
   initializeVulkan();
   initializeSwapchain();
   initializeRenderPass();
   initializeShaders();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeUniformBuffers();
   initializeMesh();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeCommandBuffers();
   initializeSyncObjects();
}

ForgeApplication::~ForgeApplication()
{
   terminateSyncObjects();
   terminateCommandBuffers(false);
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateMesh();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateShaders();
   terminateRenderPass();
   terminateSwapchain();
   terminateVulkan();
   terminateGlfw();
}

void ForgeApplication::run()
{
   while (!window->shouldClose())
   {
      window->pollEvents();
      render();
   }

   context->getDevice().waitIdle();
}

void ForgeApplication::render()
{
   if (window->pollFramebufferResized())
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }

   vk::Device device = context->getDevice();

   vk::Result frameWaitResult = device.waitForFences({ frameFences[frameIndex] }, true, UINT64_MAX);
   if (frameWaitResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to wait for frame fence");
   }

   vk::ResultValue<uint32_t> imageIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   if (imageIndexResultValue.result == vk::Result::eErrorOutOfDateKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }

      imageIndexResultValue = device.acquireNextImageKHR(swapchain->getSwapchainKHR(), UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr);
   }
   if (imageIndexResultValue.result != vk::Result::eSuccess && imageIndexResultValue.result != vk::Result::eSuboptimalKHR)
   {
      throw std::runtime_error("Failed to acquire swapchain image");
   }
   uint32_t imageIndex = imageIndexResultValue.value;

   if (imageFences[imageIndex])
   {
      // If a previous frame is still using the image, wait for it to complete
      vk::Result imageWaitResult = device.waitForFences({ imageFences[imageIndex] }, true, UINT64_MAX);
      if (imageWaitResult != vk::Result::eSuccess)
      {
         throw std::runtime_error("Failed to wait for image fence");
      }
   }
   imageFences[imageIndex] = frameFences[frameIndex];

   std::array<vk::Semaphore, 1> waitSemaphores = { imageAvailableSemaphores[frameIndex] };
   std::array<vk::PipelineStageFlags, 1> waitStages = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
   static_assert(waitSemaphores.size() == waitStages.size(), "Wait semaphores and wait stages must be parallel arrays");

   updateUniformBuffers(imageIndex);

   std::array<vk::Semaphore, 1> signalSemaphores = { renderFinishedSemaphores[frameIndex] };
   vk::SubmitInfo submitInfo = vk::SubmitInfo()
      .setWaitSemaphores(waitSemaphores)
      .setPWaitDstStageMask(waitStages.data())
      .setCommandBuffers(commandBuffers[imageIndex])
      .setSignalSemaphores(signalSemaphores);

   device.resetFences({ frameFences[frameIndex] });
   context->getGraphicsQueue().submit({ submitInfo }, frameFences[frameIndex]);

   vk::SwapchainKHR swapchainKHR = swapchain->getSwapchainKHR();
   vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
      .setWaitSemaphores(signalSemaphores)
      .setSwapchains(swapchainKHR)
      .setImageIndices(imageIndex);

   vk::Result presentResult = context->getPresentQueue().presentKHR(presentInfo);
   if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
   {
      if (!recreateSwapchain())
      {
         return;
      }
   }
   else if (presentResult != vk::Result::eSuccess)
   {
      throw std::runtime_error("Failed to present swapchain image");
   }

   frameIndex = (frameIndex + 1) % kMaxFramesInFlight;
}

void ForgeApplication::updateUniformBuffers(uint32_t index)
{
   double time = glfwGetTime();

   glm::mat4 worldToView = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

   vk::Extent2D swapchainExtent = swapchain->getExtent();
   glm::mat4 viewToClip = glm::perspective(glm::radians(70.0f), static_cast<float>(swapchainExtent.width) / swapchainExtent.height, 0.1f, 10.0f);
   viewToClip[1][1] *= -1.0f;

   ViewUniformData viewUniformData;
   viewUniformData.worldToClip = viewToClip * worldToView;

   MeshUniformData meshUniformData;
   meshUniformData.localToWorld = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f) * static_cast<float>(time), glm::vec3(0.0f, 0.0f, 1.0f));

   viewUniformBuffer->update(*context, viewUniformData, index);
   meshUniformBuffer->update(*context, meshUniformData, index);
}

bool ForgeApplication::recreateSwapchain()
{
   context->getDevice().waitIdle();

   terminateCommandBuffers(true);
   terminateDescriptorSets();
   terminateDescriptorPool();
   terminateUniformBuffers();
   terminateFramebuffers();
   terminateGraphicsPipeline();
   terminateRenderPass();
   terminateSwapchain();

   vk::Extent2D windowExtent = window->getExtent();
   while ((windowExtent.width == 0 || windowExtent.height == 0) && !window->shouldClose())
   {
      window->waitEvents();
      windowExtent = window->getExtent();
   }

   if (windowExtent.width == 0 || windowExtent.height == 0)
   {
      return false;
   }

   initializeSwapchain();
   initializeRenderPass();
   initializeGraphicsPipeline();
   initializeFramebuffers();
   initializeUniformBuffers();
   initializeDescriptorPool();
   initializeDescriptorSets();
   initializeCommandBuffers();

   return true;
}

void ForgeApplication::initializeGlfw()
{
   if (!glfwInit())
   {
      throw std::runtime_error("Failed to initialize GLFW");
   }

   window = std::make_unique<Window>();
}

void ForgeApplication::terminateGlfw()
{
   ASSERT(window);
   window = nullptr;

   glfwTerminate();
}

void ForgeApplication::initializeVulkan()
{
   context = std::make_unique<GraphicsContext>(*window);
}

void ForgeApplication::terminateVulkan()
{
   meshResourceManager.clear();
   textureResourceManager.clear();

   context = nullptr;
}

void ForgeApplication::initializeSwapchain()
{
   swapchain = std::make_unique<Swapchain>(*context, window->getExtent());

   vk::Extent2D swapchainExtent = swapchain->getExtent();
   vk::SampleCountFlagBits sampleCount = getMaxSampleCount(*context);
   {
      ImageProperties colorImageProperties;
      colorImageProperties.format = swapchain->getFormat();
      colorImageProperties.width = swapchainExtent.width;
      colorImageProperties.height = swapchainExtent.height;

      TextureProperties colorTextureProperties;
      colorTextureProperties.sampleCount = sampleCount;
      colorTextureProperties.usage = vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment;
      colorTextureProperties.aspects = vk::ImageAspectFlagBits::eColor;

      TextureInitialLayout colorInitialLayout;
      colorInitialLayout.layout = vk::ImageLayout::eColorAttachmentOptimal;
      colorInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eColorAttachmentOutput);

      colorTexture = std::make_unique<Texture>(*context, colorImageProperties, colorTextureProperties, colorInitialLayout);
   }

   {
      ImageProperties depthImageProperties;
      depthImageProperties.format = Texture::findSupportedFormat(*context, { vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD16Unorm }, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
      depthImageProperties.width = swapchainExtent.width;
      depthImageProperties.height = swapchainExtent.height;

      TextureProperties depthTextureProperties;
      depthTextureProperties.sampleCount = sampleCount;
      depthTextureProperties.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
      depthTextureProperties.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

      TextureInitialLayout depthInitialLayout;
      depthInitialLayout.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthInitialLayout.memoryBarrierFlags = TextureMemoryBarrierFlags(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::PipelineStageFlagBits::eEarlyFragmentTests);

      depthTexture = std::make_unique<Texture>(*context, depthImageProperties, depthTextureProperties, depthInitialLayout);
   }
}

void ForgeApplication::terminateSwapchain()
{
   depthTexture = nullptr;
   colorTexture = nullptr;

   swapchain = nullptr;
}

void ForgeApplication::initializeRenderPass()
{
   vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
      .setFormat(colorTexture->getImageProperties().format)
      .setSamples(colorTexture->getTextureProperties().sampleCount)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

   vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
      .setAttachment(0)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   std::array<vk::AttachmentReference, 1> colorAttachments = { colorAttachmentReference };

   vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
      .setFormat(depthTexture->getImageProperties().format)
      .setSamples(depthTexture->getTextureProperties().sampleCount)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

   vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference()
      .setAttachment(1)
      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

   vk::AttachmentDescription colorAttachmentResolve = vk::AttachmentDescription()
      .setFormat(swapchain->getFormat())
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

   vk::AttachmentReference colorAttachmentResolveReference = vk::AttachmentReference()
      .setAttachment(2)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

   std::array<vk::AttachmentReference, 1> resolveAttachments = { colorAttachmentResolveReference };

   vk::SubpassDescription subpassDescription = vk::SubpassDescription()
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(colorAttachments)
      .setPDepthStencilAttachment(&depthAttachmentReference)
      .setResolveAttachments(resolveAttachments);

   vk::SubpassDependency subpassDependency = vk::SubpassDependency()
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask({})
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

   std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
   vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
      .setAttachments(attachments)
      .setSubpasses(subpassDescription)
      .setDependencies(subpassDependency);

   renderPass = context->getDevice().createRenderPass(renderPassCreateInfo);
}

void ForgeApplication::terminateRenderPass()
{
   context->getDevice().destroyRenderPass(renderPass);
   renderPass = nullptr;
}

void ForgeApplication::initializeShaders()
{
   simpleShader = std::make_unique<SimpleShader>(shaderModuleResourceManager, *context);
}

void ForgeApplication::terminateShaders()
{
   simpleShader.reset();
   shaderModuleResourceManager.clear();
}

void ForgeApplication::initializeGraphicsPipeline()
{
   std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = simpleShader->getStages(true);

   std::array<vk::VertexInputBindingDescription, 1> vertexBindingDescriptions = Vertex::getBindingDescriptions();
   std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
   vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
      .setVertexBindingDescriptions(vertexBindingDescriptions)
      .setVertexAttributeDescriptions(vertexAttributeDescriptions);

   vk::Extent2D swapchainExtent = swapchain->getExtent();
   vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
      .setTopology(vk::PrimitiveTopology::eTriangleList);

   vk::Viewport viewport = vk::Viewport()
      .setX(0.0f)
      .setY(0.0f)
      .setWidth(static_cast<float>(swapchainExtent.width))
      .setHeight(static_cast<float>(swapchainExtent.height))
      .setMinDepth(0.0f)
      .setMaxDepth(1.0f);

   vk::Rect2D scissor = vk::Rect2D()
      .setExtent(swapchainExtent);

   vk::PipelineViewportStateCreateInfo viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
      .setViewports(viewport)
      .setScissors(scissor);

   vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eCounterClockwise);

   vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
      .setRasterizationSamples(colorTexture->getTextureProperties().sampleCount)
      .setSampleShadingEnable(true)
      .setMinSampleShading(0.2f);

   vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
      .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(false);

   vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
      .setDepthTestEnable(true)
      .setDepthWriteEnable(true)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(false)
      .setStencilTestEnable(false);

   vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
      .setLogicOpEnable(false)
      .setAttachments(colorBlendAttachmentState);

   std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = simpleShader->getSetLayouts();
   vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
      .setSetLayouts(descriptorSetLayouts);
   pipelineLayout = context->getDevice().createPipelineLayout(pipelineLayoutCreateInfo);

   vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
      .setStages(shaderStages)
      .setPVertexInputState(&vertexInputCreateInfo)
      .setPInputAssemblyState(&inputAssemblyCreateInfo)
      .setPViewportState(&viewportStateCreateInfo)
      .setPRasterizationState(&rasterizationStateCreateInfo)
      .setPMultisampleState(&multisampleStateCreateInfo)
      .setPDepthStencilState(&depthStencilCreateInfo)
      .setPColorBlendState(&colorBlendStateCreateInfo)
      .setPDynamicState(nullptr)
      .setLayout(pipelineLayout)
      .setRenderPass(renderPass)
      .setSubpass(0)
      .setBasePipelineHandle(nullptr)
      .setBasePipelineIndex(-1);

   graphicsPipeline = context->getDevice().createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;
}

void ForgeApplication::terminateGraphicsPipeline()
{
   context->getDevice().destroyPipeline(graphicsPipeline);
   graphicsPipeline = nullptr;

   context->getDevice().destroyPipelineLayout(pipelineLayout);
   pipelineLayout = nullptr;
}

void ForgeApplication::initializeFramebuffers()
{
   vk::Extent2D swapchainExtent = swapchain->getExtent();

   ASSERT(swapchainFramebuffers.empty());
   swapchainFramebuffers.reserve(swapchain->getImageCount());

   for (vk::ImageView swapchainImageView : swapchain->getImageViews())
   {
      std::array<vk::ImageView, 3> attachments = { colorTexture->getDefaultView(), depthTexture->getDefaultView(), swapchainImageView };

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
         .setWidth(swapchainExtent.width)
         .setHeight(swapchainExtent.height)
         .setLayers(1);

      swapchainFramebuffers.push_back(context->getDevice().createFramebuffer(framebufferCreateInfo));
   }
}

void ForgeApplication::terminateFramebuffers()
{
   for (vk::Framebuffer swapchainFramebuffer : swapchainFramebuffers)
   {
      context->getDevice().destroyFramebuffer(swapchainFramebuffer);
   }
   swapchainFramebuffers.clear();
}

void ForgeApplication::initializeUniformBuffers()
{
   uint32_t swapchainImageCount = swapchain->getImageCount();

   viewUniformBuffer = std::make_unique<UniformBuffer<ViewUniformData>>(*context, swapchainImageCount);
   meshUniformBuffer = std::make_unique<UniformBuffer<MeshUniformData>>(*context, swapchainImageCount);
}

void ForgeApplication::terminateUniformBuffers()
{
   meshUniformBuffer.reset();
   viewUniformBuffer.reset();
}

void ForgeApplication::initializeDescriptorPool()
{
   uint32_t swapchainImageCount = swapchain->getImageCount();

   vk::DescriptorPoolSize uniformPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(swapchainImageCount * 2);

   vk::DescriptorPoolSize samplerPoolSize = vk::DescriptorPoolSize()
      .setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(swapchainImageCount);

   std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes =
   {
      uniformPoolSize,
      samplerPoolSize
   };

   vk::DescriptorPoolCreateInfo createInfo = vk::DescriptorPoolCreateInfo()
      .setPoolSizes(descriptorPoolSizes)
      .setMaxSets(swapchainImageCount * 2);
   descriptorPool = context->getDevice().createDescriptorPool(createInfo);
}

void ForgeApplication::terminateDescriptorPool()
{
   ASSERT(!simpleShader->areDescriptorSetsAllocated());

   context->getDevice().destroyDescriptorPool(descriptorPool);
   descriptorPool = nullptr;
}

void ForgeApplication::initializeDescriptorSets()
{
   uint32_t swapchainImageCount = swapchain->getImageCount();
   simpleShader->allocateDescriptorSets(descriptorPool, swapchainImageCount);
   simpleShader->updateDescriptorSets(*context, swapchainImageCount, *viewUniformBuffer, *meshUniformBuffer, *textureResourceManager.get(textureHandle), sampler);
}

void ForgeApplication::terminateDescriptorSets()
{
   simpleShader->clearDescriptorSets();
}

void ForgeApplication::initializeMesh()
{
   meshHandle = meshResourceManager.load("Resources/Meshes/Viking/viking_room.obj", *context);
   if (!meshHandle)
   {
      throw std::runtime_error(std::string("Failed to load mesh"));
   }

   textureHandle = textureResourceManager.load("Resources/Meshes/Viking/viking_room.png", *context);
   if (!textureHandle)
   {
      throw std::runtime_error(std::string("Failed to load image"));
   }

   bool anisotropySupported = context->getPhysicalDeviceFeatures().samplerAnisotropy;
   vk::SamplerCreateInfo samplerCreateInfo = vk::SamplerCreateInfo()
      .setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
      .setAnisotropyEnable(anisotropySupported)
      .setMaxAnisotropy(anisotropySupported ? 16.0f : 1.0f)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setMipLodBias(0.0f)
      .setMinLod(0.0f)
      .setMaxLod(16.0f);
   sampler = context->getDevice().createSampler(samplerCreateInfo);
}

void ForgeApplication::terminateMesh()
{
   meshResourceManager.unload(meshHandle);
   meshHandle.reset();

   textureResourceManager.unload(textureHandle);
   textureHandle.reset();

   context->getDevice().destroySampler(sampler);
   sampler = nullptr;
}

void ForgeApplication::initializeCommandBuffers()
{
   if (!commandPool)
   {
      vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo()
         .setQueueFamilyIndex(context->getQueueFamilyIndices().graphicsFamily);

      commandPool = context->getDevice().createCommandPool(commandPoolCreateInfo);
   }

   vk::CommandBufferAllocateInfo commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
      .setCommandPool(commandPool)
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(static_cast<uint32_t>(swapchainFramebuffers.size()));

   ASSERT(commandBuffers.empty());
   commandBuffers = context->getDevice().allocateCommandBuffers(commandBufferAllocateInfo);

   const Mesh* mesh = meshResourceManager.get(meshHandle);
   ASSERT(mesh);

   for (std::size_t i = 0; i < commandBuffers.size(); ++i)
   {
      vk::CommandBuffer commandBuffer = commandBuffers[i];

      vk::CommandBufferBeginInfo commandBufferBeginInfo;

      commandBuffer.begin(commandBufferBeginInfo);

      std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
      std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColorValues), vk::ClearDepthStencilValue(1.0f, 0) };

      vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
         .setRenderPass(renderPass)
         .setFramebuffer(swapchainFramebuffers[i])
         .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), swapchain->getExtent()))
         .setClearValues(clearValues);

      commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

      simpleShader->bindDescriptorSets(commandBuffer, pipelineLayout, static_cast<uint32_t>(i));

      for (uint32_t section = 0; section < mesh->getNumSections(); ++section)
      {
         mesh->bindBuffers(commandBuffer, section);
         mesh->draw(commandBuffer, section);
      }

      commandBuffer.endRenderPass();

      commandBuffer.end();
   }
}

void ForgeApplication::terminateCommandBuffers(bool keepPoolAlive)
{
   if (keepPoolAlive)
   {
      context->getDevice().freeCommandBuffers(commandPool, commandBuffers);
   }
   else
   {
      // Command buffers will get cleaned up with the pool
      context->getDevice().destroyCommandPool(commandPool);
      commandPool = nullptr;
   }

   commandBuffers.clear();
}

void ForgeApplication::initializeSyncObjects()
{
   imageAvailableSemaphores.resize(kMaxFramesInFlight);
   renderFinishedSemaphores.resize(kMaxFramesInFlight);
   frameFences.resize(kMaxFramesInFlight);
   imageFences.resize(swapchain->getImageCount());

   vk::SemaphoreCreateInfo semaphoreCreateInfo;
   vk::FenceCreateInfo fenceCreateInfo = vk::FenceCreateInfo()
      .setFlags(vk::FenceCreateFlagBits::eSignaled);

   for (std::size_t i = 0; i < kMaxFramesInFlight; ++i)
   {
      imageAvailableSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      renderFinishedSemaphores[i] = context->getDevice().createSemaphore(semaphoreCreateInfo);
      frameFences[i] = context->getDevice().createFence(fenceCreateInfo);
   }
}

void ForgeApplication::terminateSyncObjects()
{
   for (vk::Fence frameFence : frameFences)
   {
      context->getDevice().destroyFence(frameFence);
   }
   frameFences.clear();

   for (vk::Semaphore renderFinishedSemaphore : renderFinishedSemaphores)
   {
      context->getDevice().destroySemaphore(renderFinishedSemaphore);
   }
   renderFinishedSemaphores.clear();

   for (vk::Semaphore imageAvailableSemaphore : imageAvailableSemaphores)
   {
      context->getDevice().destroySemaphore(imageAvailableSemaphore);
   }
   imageAvailableSemaphores.clear();
}
