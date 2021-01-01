#include "Renderer/Passes/Depth/DepthPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Depth/DepthShader.h"
#include "Renderer/UniformData.h"

DepthPass::DepthPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& depthTexture)
   : GraphicsResource(graphicsContext)
{
   depthShader = std::make_unique<DepthShader>(context, resourceManager);

   initializeSwapchainDependentResources(depthTexture);
}

DepthPass::~DepthPass()
{
   terminateSwapchainDependentResources();

   depthShader.reset();
}

void DepthPass::render(vk::CommandBuffer commandBuffer, const View& view, const Mesh& mesh, const glm::mat4& localToWorld)
{
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffer)
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

   depthShader->bindDescriptorSets(commandBuffer, view, pipelineLayout);

   MeshUniformData meshUniformData;
   meshUniformData.localToWorld = localToWorld;
   commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

   for (uint32_t section = 0; section < mesh.getNumSections(); ++section)
   {
      mesh.bindBuffers(commandBuffer, section);
      mesh.draw(commandBuffer, section);
   }

   commandBuffer.endRenderPass();
}

void DepthPass::onSwapchainRecreated(const Texture& depthTexture)
{
   terminateSwapchainDependentResources();
   initializeSwapchainDependentResources(depthTexture);
}

void DepthPass::updateDescriptorSets(const View& view)
{
   ASSERT(depthShader);
   depthShader->updateDescriptorSets(view);
}

void DepthPass::initializeSwapchainDependentResources(const Texture& depthTexture)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();

   {
      vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
         .setFormat(depthTexture.getImageProperties().format)
         .setSamples(depthTexture.getTextureProperties().sampleCount)
         .setLoadOp(vk::AttachmentLoadOp::eClear)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
         .setInitialLayout(vk::ImageLayout::eUndefined)
         .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      vk::AttachmentReference depthAttachmentReference = vk::AttachmentReference()
         .setAttachment(0)
         .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      vk::SubpassDescription subpassDescription = vk::SubpassDescription()
         .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
         .setPDepthStencilAttachment(&depthAttachmentReference);

      vk::SubpassDependency subpassDependency = vk::SubpassDependency()
         .setSrcSubpass(VK_SUBPASS_EXTERNAL)
         .setDstSubpass(0)
         .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
         .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
         .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests)
         .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

      vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
         .setAttachments(depthAttachment)
         .setSubpasses(subpassDescription)
         .setDependencies(subpassDependency);

      renderPass = device.createRenderPass(renderPassCreateInfo);
   }

   {
      std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions = Vertex::getBindingDescriptions();
      std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions = Vertex::getAttributeDescriptions(true);
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
         .setRasterizationSamples(depthTexture.getTextureProperties().sampleCount)
         .setSampleShadingEnable(true)
         .setMinSampleShading(0.2f);

      vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
         .setDepthTestEnable(true)
         .setDepthWriteEnable(true)
         .setDepthCompareOp(vk::CompareOp::eLess)
         .setDepthBoundsTestEnable(false)
         .setStencilTestEnable(false);

      vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
         .setLogicOpEnable(false);

      std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = depthShader->getStages();
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = depthShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = depthShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
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
      std::array<vk::ImageView, 1> attachments = { depthTexture.getDefaultView() };

      vk::FramebufferCreateInfo framebufferCreateInfo = vk::FramebufferCreateInfo()
         .setRenderPass(renderPass)
         .setAttachments(attachments)
         .setWidth(swapchainExtent.width)
         .setHeight(swapchainExtent.height)
         .setLayers(1);

      framebuffer = device.createFramebuffer(framebufferCreateInfo);
   }
}

void DepthPass::terminateSwapchainDependentResources()
{
   if (framebuffer)
   {
      device.destroyFramebuffer(framebuffer);
      framebuffer = nullptr;
   }

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

