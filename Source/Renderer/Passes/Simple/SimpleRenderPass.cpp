#include "Renderer/Passes/Simple/SimpleRenderPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Simple/SimpleShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

SimpleRenderPass::SimpleRenderPass(const GraphicsContext& graphicsContext, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture)
   : GraphicsResource(graphicsContext)
{
   simpleShader = std::make_unique<SimpleShader>(context, resourceManager);

   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

SimpleRenderPass::~SimpleRenderPass()
{
   terminateSwapchainDependentResources();

   simpleShader.reset();
}

void SimpleRenderPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColorValues), vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffers[context.getSwapchainIndex()])
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

   for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
   {
      MeshUniformData meshUniformData;
      meshUniformData.localToWorld = meshRenderInfo.localToWorld;
      commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

      for (uint32_t section = 0; section < meshRenderInfo.mesh.getNumSections(); ++section)
      {
         if (const Material* material = meshRenderInfo.materials[section])
         {
            simpleShader->bindDescriptorSets(commandBuffer, sceneRenderInfo.view, pipelineLayout, *material);

            meshRenderInfo.mesh.bindBuffers(commandBuffer, section);
            meshRenderInfo.mesh.draw(commandBuffer, section);
         }
      }
   }

   commandBuffer.endRenderPass();
}

void SimpleRenderPass::onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture)
{
   terminateSwapchainDependentResources();
   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

void SimpleRenderPass::initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture)
{
   vk::Extent2D swapchainExtent = context.getSwapchain().getExtent();
   bool isMultisampled = colorTexture.getTextureProperties().sampleCount != vk::SampleCountFlagBits::e1;

   {
      vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
         .setFormat(isMultisampled ? colorTexture.getImageProperties().format : context.getSwapchain().getFormat())
         .setSamples(isMultisampled ? colorTexture.getTextureProperties().sampleCount : vk::SampleCountFlagBits::e1)
         .setLoadOp(vk::AttachmentLoadOp::eClear)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
         .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
         .setInitialLayout(vk::ImageLayout::eUndefined)
         .setFinalLayout(isMultisampled ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::ePresentSrcKHR);

      vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
         .setAttachment(0)
         .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

      std::array<vk::AttachmentReference, 1> colorAttachments = { colorAttachmentReference };

      vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
         .setFormat(depthTexture.getImageProperties().format)
         .setSamples(depthTexture.getTextureProperties().sampleCount)
         .setLoadOp(vk::AttachmentLoadOp::eLoad)
         .setStoreOp(vk::AttachmentStoreOp::eStore)
         .setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
         .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
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
         .setPDepthStencilAttachment(&depthAttachmentReference);

      if (isMultisampled)
      {
         subpassDescription.setResolveAttachments(resolveAttachments);
      }

      vk::SubpassDependency subpassDependency = vk::SubpassDependency()
         .setSrcSubpass(VK_SUBPASS_EXTERNAL)
         .setDstSubpass(0)
         .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
         .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
         .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
         .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

      std::vector<vk::AttachmentDescription> attachments = { colorAttachment, depthAttachment };
      if (isMultisampled)
      {
         attachments.push_back(colorAttachmentResolve);
      }
      vk::RenderPassCreateInfo renderPassCreateInfo = vk::RenderPassCreateInfo()
         .setAttachments(attachments)
         .setSubpasses(subpassDescription)
         .setDependencies(subpassDependency);

      renderPass = device.createRenderPass(renderPassCreateInfo);
   }

   {
      std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions = Vertex::getBindingDescriptions();
      std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions = Vertex::getAttributeDescriptions(false);
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
         .setRasterizationSamples(isMultisampled ? colorTexture.getTextureProperties().sampleCount : vk::SampleCountFlagBits::e1)
         .setSampleShadingEnable(true)
         .setMinSampleShading(0.2f);

      vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
         .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
         .setBlendEnable(false);

      vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
         .setDepthTestEnable(true)
         .setDepthWriteEnable(false)
         .setDepthCompareOp(vk::CompareOp::eEqual)
         .setDepthBoundsTestEnable(false)
         .setStencilTestEnable(false);

      vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
         .setLogicOpEnable(false)
         .setAttachments(colorBlendAttachmentState);

      std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = simpleShader->getStages(true);
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = simpleShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = simpleShader->getPushConstantRanges();
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
      framebuffers.reserve(context.getSwapchain().getImageCount());

      for (vk::ImageView swapchainImageView : context.getSwapchain().getImageViews())
      {
         std::vector<vk::ImageView> attachments;
         if (isMultisampled)
         {
            attachments = { colorTexture.getDefaultView(), depthTexture.getDefaultView(), swapchainImageView };
         }
         else
         {
            attachments = { swapchainImageView, depthTexture.getDefaultView() };
         }

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
