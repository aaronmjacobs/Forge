#include "Renderer/Passes/Simple/SimpleRenderPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Simple/SimpleShader.h"

#include "Resources/ResourceManager.h"

SimpleRenderPass::SimpleRenderPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture)
   : GraphicsResource(graphicsContext)
{
   {
      simpleShader = std::make_unique<SimpleShader>(context, resourceManager);
   }

   {
      bool anisotropySupported = context.getPhysicalDeviceFeatures().samplerAnisotropy;
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
      sampler = device.createSampler(samplerCreateInfo);
   }

   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

SimpleRenderPass::~SimpleRenderPass()
{
   terminateSwapchainDependentResources();

   device.destroySampler(sampler);

   simpleShader.reset();
}

void SimpleRenderPass::render(vk::CommandBuffer commandBuffer, uint32_t swapchainIndex, const Mesh& mesh)
{
   vk::CommandBufferBeginInfo commandBufferBeginInfo;
   commandBuffer.begin(commandBufferBeginInfo);

   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColorValues), vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffers[swapchainIndex])
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

   simpleShader->bindDescriptorSets(commandBuffer, pipelineLayout, swapchainIndex);

   for (uint32_t section = 0; section < mesh.getNumSections(); ++section)
   {
      mesh.bindBuffers(commandBuffer, section);
      mesh.draw(commandBuffer, section);
   }

   commandBuffer.endRenderPass();
}

void SimpleRenderPass::onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture)
{
   terminateSwapchainDependentResources();
   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

bool SimpleRenderPass::areDescriptorSetsAllocated() const
{
   return simpleShader && simpleShader->areDescriptorSetsAllocated();
}

void SimpleRenderPass::allocateDescriptorSets(vk::DescriptorPool descriptorPool)
{
   ASSERT(simpleShader);
   simpleShader->allocateDescriptorSets(descriptorPool);
}

void SimpleRenderPass::clearDescriptorSets()
{
   ASSERT(simpleShader);
   simpleShader->clearDescriptorSets();
}

void SimpleRenderPass::updateDescriptorSets(const UniformBuffer<ViewUniformData>& viewUniformBuffer, const UniformBuffer<MeshUniformData>& meshUniformBuffer, const Texture& texture)
{
   ASSERT(simpleShader);
   simpleShader->updateDescriptorSets(viewUniformBuffer, meshUniformBuffer, texture, sampler);
}

void SimpleRenderPass::initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   {
      vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
         .setFormat(colorTexture.getImageProperties().format)
         .setSamples(colorTexture.getTextureProperties().sampleCount)
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
         .setFormat(depthTexture.getImageProperties().format)
         .setSamples(depthTexture.getTextureProperties().sampleCount)
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
         .setFormat(context.getSwapchain().getFormat())
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

      renderPass = device.createRenderPass(renderPassCreateInfo);
   }

   {
      std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = simpleShader->getStages(true);

      std::array<vk::VertexInputBindingDescription, 1> vertexBindingDescriptions = Vertex::getBindingDescriptions();
      std::array<vk::VertexInputAttributeDescription, 3> vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
      vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::PipelineVertexInputStateCreateInfo()
         .setVertexBindingDescriptions(vertexBindingDescriptions)
         .setVertexAttributeDescriptions(vertexAttributeDescriptions);

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
         .setRasterizationSamples(colorTexture.getTextureProperties().sampleCount)
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
      pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

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

      pipeline = device.createGraphicsPipeline(nullptr, graphicsPipelineCreateInfo).value;
   }

   {
      framebuffers.reserve(context.getSwapchain().getImageCount());

      for (vk::ImageView swapchainImageView : context.getSwapchain().getImageViews())
      {
         std::array<vk::ImageView, 3> attachments = { colorTexture.getDefaultView(), depthTexture.getDefaultView(), swapchainImageView };

         vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
            .setRenderPass(renderPass)
            .setAttachments(attachments)
            .setWidth(swapchainExtent.width)
            .setHeight(swapchainExtent.height)
            .setLayers(1);

         framebuffers.push_back(device.createFramebuffer(framebufferCreateInfo));
      }
   }
}

void SimpleRenderPass::terminateSwapchainDependentResources()
{
   for (vk::Framebuffer framebuffer : framebuffers)
   {
      device.destroyFramebuffer(framebuffer);
   }
   framebuffers.clear();

   if (pipeline)
   {
      device.destroyPipeline(pipeline);
      pipeline = nullptr;
   }
   if (pipelineLayout)
   {
      device.destroyPipelineLayout(pipelineLayout);
      pipelineLayout = nullptr;
   }

   if (renderPass)
   {
      device.destroyRenderPass(renderPass);
      renderPass = nullptr;
   }
}
