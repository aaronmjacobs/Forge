#include "Renderer/Passes/Forward/ForwardRenderPass.h"

#include "Graphics/Mesh.h"
#include "Graphics/Pipeline.h"
#include "Graphics/Swapchain.h"
#include "Graphics/Texture.h"

#include "Renderer/Passes/Forward/ForwardShader.h"
#include "Renderer/SceneRenderInfo.h"
#include "Renderer/UniformData.h"

namespace
{
   uint32_t getForwardPipelineIndex(bool withTextures, bool withBlending)
   {
      return (withTextures ? 0b01 : 0b00) | (withBlending ? 0b10 : 0b00);
   }
}

ForwardRenderPass::ForwardRenderPass(const GraphicsContext& graphicsContext, vk::DescriptorPool descriptorPool, ResourceManager& resourceManager, const Texture& colorTexture, const Texture& depthTexture)
   : GraphicsResource(graphicsContext)
   , lighting(graphicsContext, descriptorPool)
{
   forwardShader = std::make_unique<ForwardShader>(context, resourceManager);

   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

ForwardRenderPass::~ForwardRenderPass()
{
   terminateSwapchainDependentResources();

   forwardShader.reset();
}

void ForwardRenderPass::render(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
   std::array<vk::ClearValue, 2> clearValues = { vk::ClearColorValue(clearColorValues), vk::ClearDepthStencilValue(1.0f, 0) };

   vk::RenderPassBeginInfo renderPassBeginInfo = vk::RenderPassBeginInfo()
      .setRenderPass(renderPass)
      .setFramebuffer(framebuffers[context.getSwapchainIndex()])
      .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), context.getSwapchain().getExtent()))
      .setClearValues(clearValues);
   commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

   lighting.update(sceneRenderInfo);

   renderMeshes<false>(commandBuffer, sceneRenderInfo);
   renderMeshes<true>(commandBuffer, sceneRenderInfo);

   commandBuffer.endRenderPass();
}

void ForwardRenderPass::onSwapchainRecreated(const Texture& colorTexture, const Texture& depthTexture)
{
   terminateSwapchainDependentResources();
   initializeSwapchainDependentResources(colorTexture, depthTexture);
}

void ForwardRenderPass::initializeSwapchainDependentResources(const Texture& colorTexture, const Texture& depthTexture)
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
      std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = forwardShader->getSetLayouts();
      std::vector<vk::PushConstantRange> pushConstantRanges = forwardShader->getPushConstantRanges();
      vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
         .setSetLayouts(descriptorSetLayouts)
         .setPushConstantRanges(pushConstantRanges);
      pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

      std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesWithoutTextures = forwardShader->getStages(false);
      std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesWithTextures = forwardShader->getStages(true);

      vk::PipelineColorBlendAttachmentState colorBlendDisabledAttachmentState = vk::PipelineColorBlendAttachmentState()
         .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
         .setBlendEnable(false);
      vk::PipelineColorBlendAttachmentState colorBlendEnabledAttachmentState = vk::PipelineColorBlendAttachmentState()
         .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
         .setBlendEnable(true)
         .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
         .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
         .setColorBlendOp(vk::BlendOp::eAdd)
         .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
         .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
         .setAlphaBlendOp(vk::BlendOp::eAdd);
      std::vector<vk::PipelineColorBlendAttachmentState> colorBlendDisabledAttachments = { colorBlendDisabledAttachmentState };
      std::vector<vk::PipelineColorBlendAttachmentState> colorBlendEnabledAttachments = { colorBlendEnabledAttachmentState };

      PipelineData pipelineData(context, pipelineLayout, renderPass, shaderStagesWithoutTextures, colorBlendDisabledAttachments, colorTexture.getTextureProperties().sampleCount);
      pipelines[getForwardPipelineIndex(false, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

      pipelineData.setShaderStages(shaderStagesWithTextures);
      pipelines[getForwardPipelineIndex(true, false)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

      pipelineData.setColorBlendAttachmentStates(colorBlendEnabledAttachments);
      pipelines[getForwardPipelineIndex(true, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;

      pipelineData.setShaderStages(shaderStagesWithoutTextures);
      pipelines[getForwardPipelineIndex(false, true)] = device.createGraphicsPipeline(nullptr, pipelineData.getCreateInfo()).value;
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

void ForwardRenderPass::terminateSwapchainDependentResources()
{
   for (vk::Framebuffer framebuffer : framebuffers)
   {
      device.destroyFramebuffer(framebuffer);
   }
   framebuffers.clear();

   for (vk::Pipeline& pipeline : pipelines)
   {
      if (pipeline)
      {
         device.destroyPipeline(pipeline);
         pipeline = nullptr;
      }
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

template<bool translucency>
void ForwardRenderPass::renderMeshes(vk::CommandBuffer commandBuffer, const SceneRenderInfo& sceneRenderInfo)
{
   vk::Pipeline lastPipeline;
   for (const MeshRenderInfo& meshRenderInfo : sceneRenderInfo.meshes)
   {
      const std::vector<uint32_t>& sections = translucency ? meshRenderInfo.visibleTranslucentSections : meshRenderInfo.visibleOpaqueSections;
      if (!sections.empty())
      {
         ASSERT(meshRenderInfo.mesh);

         MeshUniformData meshUniformData;
         meshUniformData.localToWorld = meshRenderInfo.localToWorld;
         commandBuffer.pushConstants<MeshUniformData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, meshUniformData);

         for (uint32_t section : sections)
         {
            const Material* material = meshRenderInfo.materials[section];
            ASSERT(material);

            vk::Pipeline desiredPipeline = selectPipeline(meshRenderInfo.mesh->getSection(section), *material);
            if (desiredPipeline != lastPipeline)
            {
               commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, desiredPipeline);
               lastPipeline = desiredPipeline;
            }

            forwardShader->bindDescriptorSets(commandBuffer, pipelineLayout, sceneRenderInfo.view, lighting, *material);

            meshRenderInfo.mesh->bindBuffers(commandBuffer, section);
            meshRenderInfo.mesh->draw(commandBuffer, section);
         }
      }
   }
}

vk::Pipeline ForwardRenderPass::selectPipeline(const MeshSection& meshSection, const Material& material) const
{
   return pipelines[getForwardPipelineIndex(meshSection.hasValidTexCoords, material.getBlendMode() == BlendMode::Translucent)];
}
